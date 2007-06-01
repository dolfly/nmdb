
=====================================
nmdb - A TIPC-based database manager
=====================================
:Author: Alberto Bertogli (albertito@gmail.com)


Introduction
============

nmdb_ is a simple and fast cache and database for TIPC clusters. It allows
applications in the cluster to use a centralized, shared cache and database in
a very easy way. It stores *(key, value)* pairs, with each key having only one
associated value.

This document explains how the server works internally, and why it works that
way.


Network interface
=================

The server communicates with its clients using messages, which can be
delivered through TIPC_ or TCP. Messages are limited by design to 64k so they
stay inside within TIPC_'s limits.

TIPC_ is completely connectionless, and uses the reliable datagram layer
provided by TIPC_. The network protocol is specified in another document, and
will not be subject of analysis here.

The interaction is very simple: the client sends a request message for an
action, and the server replies to it. There is only one message per request,
and only one message per reply. This imposes a limit on the size of keys and
values, which is fixed at 64Kb for each, and for their concatenation. That
means each key or value must be shorter than 64Kb, and the sum of their sizes
must be shorter than 64Kb too.

There are several requests that can be made to the server:

get *key*
  Retrieves the value for the given key. If the key is in the cache, it
  returns immediately. If not, it performs a query in the database.

set_async *key* *value*
  Stores the *(key, value)* pair in the database. It does the set in the cache,
  queues the operation for the database, and returns.

del_async *key*
  Removes the key and it's associated value from the database. It does the del
  in the cache, queues the operation for the database, and returns.

set_sync *key* *value*
  Like *set*, but return only after the database has completed the operation.

del_sync *key*
  Like *del*, but return only after the database has completed the operation.

cache_get *key*
  Like *get*, but only affects the cache and not the database. If the key is
  not in the cache, returns a special value indicating "miss".

cache_set *key* *value*
  Like *set*, but only affects the cache and not the database.

cache_del *key*
  Like *del*, but only affects the cache and not the database.

cache_cas *key* *oldvalue* *newvalue*
  Do a compare-and-swap, using *oldvalue* to compare with the value stored in
  the database, and replacing it with *newvalue* if they match.

As you can see, it's possible to operate exclusively with the cache, ignoring
the database completely. This is very similar to what memcached_ does. Note
that the downside is that it's possible to mess with the cache, and leave it
out of sync with the database. You can only do this if you mix *cache_set*
with *set* or *set_sync*, which is hard to miss, so it's unlikely you will do
this.

The server assumes you have a brain, and you will not make a mess out of it.


Request processing
==================

The server consist of two threads: the main thread (or event thread), and the
database thread.

The main thread is event-based, using libevent_ for network polling, and
acting on incoming messages. Each message goes through an initial decoding
stage, and then depending on the requested command, different functions are
invoked, all which go through the same basic steps.

The database thread waits on an operation queue for operations to perform. The
operations are added by the main thread, and removed by the database thread.
When an operation appears, it process it by invoking the corresponding
database functions, and goes back to wait. This is completely synchronous, and
all the operations are processed in order.

After a request has been received, the steps performed to execute it are:

#. Do some basic sanity checking in message parsing
#. Act upon the cache. If it's a cache operation, send the reply and it's done.
#. Queue the request in the operation queue
#. If the operation is asynchronous, send the reply and it's done.
#. If not, signal the database it has work to do and it's done.


Note that for asynchronous operations the signal is not sent. This is because
sending the signal is a significant operation, and avoiding it reduces the
latency. It also means that the database might take a while in noticing the
operation, but that's not usually a problem (after all, that's why the request
was asynchronous in the first place). The signalling method chosen is a
conditional mutex, on which the database thread waits if the queue is empty.

While some operations are asynchronous, they are always processed in order. If
an application issues two operations in a row, they're guaranteed to be
completed in order. This avoids "get after del/set" issues that would
complicate the application using the database.


The database thread
===================

There is only one database thread, which makes the overall design much simpler
(there are no races between different operations, as they're all executed in
order), but reduces the potential synchronous performance.

This trade-off was chosen on the basis that most applications will use
asynchronous operations, and most DBMs do not support multithreading
operation. A specific solution could have been used, and the database backend
code is isolated enough to allow this to happen in the future if necessity
arises.

QDBM_ was chosen for the backend, because its speed, although most DBMs would
have been equally useful regarding features, because the server is not at all
demanding.

The processing is performed by taking requests from the aforementioned queue,
and acting upon the database accordingly, which involves calling the backend's
get, set or del depending on the operation in question. Then, if the operation
was synchronous, a response is sent to the client.

As mentioned in the previous section, a conditional mutex is used for
notification. When the queue is not empty, the thread waits upon it until the
main thread wakes it up. This provides low latency wakeups when necessary
(synchronous operations, specially get which is quite common), and very low
CPU usage when the database is idle.


Passive mode
============

The server has a special mode, *passive mode*, where it listen to requests,
acts upon them internally, but never sends any replies. It is used for
redundancy purposes, allowing the administrator to have an up-to-date copy of
the database in case the main one fails.

It only makes sense if used with TIPC_ because it can multicast messages.

The implementation is quite simple, because the code paths are exactly the
same, with the exception of skipping the network replies, so they're done
conditionally depending on the passive setting.

Live switching of a server from passive to active (and vice-versa) can be done
at runtime by sending a *SIGUSR2* signal to the server.


The cache layer
===============

The cache layer is implemented by a modified hash table, to make eviction
efficient and cheap.

The hash table is quite normal: several buckets (the size is decided at
initialization time), and each bucket containing a linked list with the
objects assigned to it.

There a some tricks, though:

- In order to keep a bound on the number of objects in the cache, the number
  of elements in each linked list is limited to 4.
- Whenever a lookup is made, the entry that matched is promoted to the head of
  the list containing it.
- When inserting a new element in the cache, it's always inserted to the top
  of the list, as its first element.
- When there is excess on the number of elements in the list, the bottom one
  is removed.

This causes a natural *LRU* behaviour on each list, which is quite desirable for
a cache of this kind. The size of the linked lists was chosen to be short
enough to keep lookups fast, but long enough for the *LRU* mechanism to be
useful.

If two "hot" objects were to end up in the same bucket, the cache will behave
properly, because the chances of them being evicted by a third "cold" object
are pretty low. Under stress, cold objects move to the bottom of the list
fast, so the cache does not misbehave easily.

This makes the choice of inserting new objects to the top an easy one. In
other cache implementations, adding new objects as "hot" is dangerous because
it might be easy for them to cause unwanted evictions; but on the other hand
some workloads perform better if the new entries are well ranked. Here, due to
the list size it's quite difficult for it to cause a hot object to be evicted,
so it's not a problem.

Nonetheless, it's advisable to use a large cache size, specially if the usage
pattern involves handling lots of different keys.


.. _nmdb: http://auriga.wearlab.de/~alb/nmdb/
.. _libevent: http://www.monkey.org/~provos/libevent/
.. _TIPC: http://tipc.sf.net
.. _memcached: http://www.danga.com/memcached/
.. _QDBM: http://qdbm.sf.net

