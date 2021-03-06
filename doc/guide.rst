
================
nmdb User Guide
================
:Author: Alberto Bertogli (albertito@blitiri.com.ar)


Introduction
============

nmdb_ is a simple and fast cache and database for controlled networks.
It allows applications in the network to use a centralized, shared cache and
database in a very easy way. It stores *(key, value)* pairs, with each key
having only one associated value. At the moment, it supports the TIPC_, TCP,
UDP and SCTP protocols.

This document explains how to setup nmdb and a simple guide to writing
clients. It also includes a "quick start" section for the anxious.


Installing nmdb
===============

If you installed nmdb using your Linux distribution package system, you can
skip this section entirely.


Prerequisites
-------------

Before you install nmdb, you will need the following software:

- libevent_, a library for fast event handling.
- One or more of QDBM_, BDB_, tokyocabinet_ or tdb_ for the database backend.
- If you want TIPC_ support, `Linux kernel`_ 2.6.16 or newer.
- If you want SCTP_ support, libsctp-dev (or equivalent package).


Compiling and installing
------------------------

There are three components of the nmdb tarball: the server in the *nmdb/*
directory, the C library in *libnmdb/*, and the Python module in
*bindings/python/*.

To install the server and the C library, run ``make install; ldconfig``. To
install the Python module, run ``make python_install`` after installing the C
library.

The build system autodetects the available backends and protocols, but if you
want to manually override it, you can you can do so by running, for example,
``make ENABLE_TIPC=0 BE_ENABLE_BDB=1 install`` to disable TIPC and enable BDB.


Quick start
===========

For a very quick start, using a single host, you can do the following::

  # nmdb -d /tmp/nmdb-db        # start the server, use the given database

At this point you have created a database and started the server. An easy and
simple way to test it is to use the python module, like this::

  # python
  Python 2.5 (r25:51908, Sep 21 2006, 20:38:23)
  [GCC 4.1.1 (Gentoo 4.1.1)] on linux2
  Type "help", "copyright", "credits" or "license" for more information.
  >>> import nmdb               # import the module
  >>> db = nmdb.DB()            # create a DB object
  >>> db.add_tcp_server("localhost")  # connect to the TCP server
  >>> db['x'] = 1               # store some data
  >>> db[(1, 2)] = (2, 6)
  >>> print db['x'], db[(1, 2)] # retreive the values
  1 (2, 6)
  >>> del db['x']               # delete from the database

Everything should have worked as shown, and you are now ready to use some
nmdb application, or develop your own.

If you want to use this with several machines, read the next section to find
out how to setup a simple TIPC cluster.


TIPC setup
==========

If you want to use the server and the clients in different machines using
TIPC, you need to setup your TIPC network. If you just want to run everything
in one machine, you already have a TIPC network set up, or you only want to
use TCP, UDP or SCTP connections, you can skip this section.

Before we begin, all the machines should already be connected in an Ethernet
LAN, and have the tipc-config application that should come with your Linux
distribution with a package named "tipcutils" or similar (if it doesn't, you
can find it at http://tipc.sourceforge.net/download.html).

The only thing you will need to do is assign each machine a TIPC address and
specify which interface to use for the network connection. You do it like
this::

  # tipc-config -a=1.1.10 -be=eth:eth0

The *-a* parameter specifies the address, and *-be* the type and name of the
interface to use.

Addresses are composed of three integers. They represent the zone number, the
cluster number, and the node number respectively. The zone number and cluster
number should be the same for all nodes in your network, so you should change
the last one for each machine. Each machine can have only one address.

That should be enough to get you started for a small network. If you have a
very big network, or want to use some of the advanced TIPC features like link
redundancy, you should read TIPC's docs.


Example
-------

If you have five machines, you can assign each one their address like this::

  box1# tipc-config -a=1.1.1 -be=eth:eth0
  box2# tipc-config -a=1.1.2 -be=eth:eth0
  box3# tipc-config -a=1.1.3 -be=eth:eth0
  box4# tipc-config -a=1.1.4 -be=eth:eth0
  box5# tipc-config -a=1.1.5 -be=eth:eth0


Starting the server
===================

Before starting the server, there are some things you need to know about it:

Cache size
  nmdb's cache is a main component of the server. In fact you can use it
  exclusively for caching purposes, like memcached_. So the size becomes an
  important issue if you have performance requirements.

  It is only possible to limit the cache size by the maximum number of objects
  in it, and not by byte size.

Backend database
  The backend database engine can be selected at run time via a command-line
  option.

  If for some reason (hardware failure, for instance) the database becomes
  corrupt, you should use your database utilities to fix it. It shouldn't
  happen, so it's a good idea to report it if it does.

  Most databases are not meant to be shared among processes, so avoid having
  other processes using them.

Database redundancy
  If you want to have redundancy over the database, you can start a "passive
  server" along a normal one using the same port number. It will listen to
  database requests and act upon them, but it will not reply anything.

  It is only useful to keep a live mirror of the database. Note that it does
  not do replication or failure detection, it's just a mirror.

  This is the only case where you want to start two servers with the same port.

Distributed queries
  If you have more than one server in the network, the library can distribute
  the queries among them. This is entirely done on the client side and the
  server doesn't know about it.

TIPC Port numbers
  With TIPC, each server instance in your network (even the ones running in
  the same machine) should get a **unique** port to listen to requests. Ports
  identify an application instance inside the whole network, not just the
  machine as in TCP/IP.

  The port space is very very large, and it's private to nmdb, so you can
  choose numbers without fear of colliding with other TIPC applications. The
  default port is 10.

  So, if you are going to start more than one nmdb server, **be careful**. If
  you assign two active servers the same port you will get no error, but
  everything will act weird.


Now that you know all that, starting a server should be quite simple: just run
the daemon with ``nmdb -d /path/to/the/database``.

There are several options you can change at start time. Of course you won't
remember all that (I know I don't), so check out ``nmdb -h`` to see a complete
list.

Nothing prevents you from starting more than one TIPC server in the same
machine, so be careful to select different TIPC ports and databases for each
one.


Example
-------

Following the previous example, if you want to start three servers you can do
it like this::

  box1# nmdb -d /var/lib/nmdb/db-1 -l 11
  box2# nmdb -d /var/lib/nmdb/db-2 -l 12
  box3# nmdb -d /var/lib/nmdb/db-3 -l 13


Writing clients
===============

At the moment you can write clients in C (documented in the *libnmdb*'s
manpage) and in Python (documented using Python docstrings). In this guide we
will give some examples of common use as an introduction, you should consult
the appropriate documentation when doing serious development.

Before we begin, you should know about the following things:

Thread safety
  While the library itself is thread safe, neither the C library connections
  nor the Python objects are. So don't share *nmdb_t* variables (C) or
  *nmdb.** objects (Python) among threads; instead, create one for each thread
  that needs it.

Available operations
  You can request the server to do five operations: *set* a value to a key,
  *get* the value associated with the given key, *delete* a given key (with
  its associated value), perform a *compare-and-swap* of the values associated
  with the given key, and (atomically) *increment* the value associated with
  the given key.

Request modes
  For each operation, you will have three different modes available:

  - A *normal mode*, which makes the operation impact on the database
    asynchronously (i.e. the functions return right after the operation was
    queued, there is no completion notification).
  - A *synchronous mode* similar to the previous one, but when the functions
    return, the operation has hit the disk.
  - A *cache-only mode* where the operations do not impact the database, only
    the cache, and can be used to implement distributed caching in a similar
    way to memcached_.

  Be careful with the last one, because mixing cache-only with database
  operations is a recipe for disaster.

Atomicity and coherence
  All operations are atomic, and synchronous and asynchronous operations are
  fully coherent.

Distributed queries
  You can distribute your queries among several servers, and this is entirely
  done on the client side. To do this, you should add each server (identified
  by their port numbers) to the connection **before beginning to interact with
  them**.


For all examples we will assume that you have three servers running in your
network, two in TIPC ports 11 and 12, and one TCP listening on localhost on
the default port.


The Python module
-----------------

The Python module it's quite easy to use, because its interface is very
similar to a dictionary. It has similar limitations regarding the key (it must
be an object you can use as a key in a dictionary), and the values must be
pickable objects (see the *pickle* module documentation for more information).
In short, you should only use number, strings or tuples as keys, and simple
objects as values, unless you know what you are doing.

To start a connection to the servers, you must first decide which mode you are
going to use: the normal database-backed mode, database-backed with
synchronous access, or cache only. Let's say you want to use the normal mode
and connect to the TIPC servers at port 11, 12, and a TCP server on localhost
at the default port::

  import nmdb
  db = nmdb.DB()
  db.add_tipc_server(11)
  db.add_tipc_server(12)
  db.add_tcp_server("127.0.0.1")

Now you're ready to use it. Let's suppose you want to write a recursive
function to calculate the factorial of a number. But before doing the
calculation, you can check if the previous factorial already is in the
database to avoid recalculating it::

  def fact(n):
      if n == 1:
          return 1
      if db.has_key(n):
          return db[n]

      result = n * fact(n - 1)
      db[n] = result
      return result

That was easy, wasn't it? You can use the same trick for SQL queries, complex
distributed calculations, geographical data processing, whatever you want.

Now let's have some fun and do something a little advanced: a decorator for a
distributed function cache. If Python magic scares you, look away and skip to
the next section.

Some functions (usually the mathematical ones) have the property that the
value they return depends only on the parameters, and not on the context.  So
they can be cached, using the parameters as keys, with the function's result
as their associated values. Applying this technique is commonly known as
*memoization*, and when we apply it to a function we say we're *memoizing* it.

We can use a local dictionary to cache the data, but that would mean we would
have to write some cache management code to avoid using too much memory, and,
worst of all, each instance of the code running in the network would have its
own private cache and can't reuse calculations performed by other instances.
Instead, we can use nmdb to make a cache that is shared among the network.

The functions are usually restricted to using simple types as input, like
numbers, strings, tuples or dictionaries. We will take advantage of this by
using as a key to the cache the string ``<function module>-<function
name>-<string representation of the arguments>``. So to cache an invocation
like ``mod.f(1, (2, 6))`` that returns ``26``, we want to have the following
association in the database: ``mod-f-(1, (2, 6)) = 26``.

We will use nmdb in cache-only mode, where the things we store are not saved
permanently to a database, but live in the server's memory. This is very
similar to what we did before, and has the advantage of not having to write
our own cache management routines::

  import nmdb
  db = nmdb.Cache()
  db.add_tipc_server(11)
  db.add_tipc_server(12)
  db.add_tcp_server("127.0.0.1")

Let's write the decorator::

  def shared_memoize(f):
      def newf(*args, **kwargs):
          key = '%s-%s-%s-%s' % (f.__module__, f.__name__,
                                 repr(args), repr(kwargs))
          if key in db:
              return db[key]
          r = f(*args, **kwargs)
          db[key] = r
          return r
      return newf

Now we can use it with a normal implementation of the recursive factorial
function like we did before, and a function that calculates tetrations_::

  @shared_memoize
  def fact(n):
      if n == 1:
          return 1
      return n * fact(n - 1)

  @shared_memoize
  def tetration(a, b):
      if b == 1:
          return a
      return pow(a, tetration(a, b - 1))

As you can see, the module is very easy to use, but you can do useful things
with it. For more information you can read the module's built-in
documentation.


The C library
-------------

The C library is in essence similar to the Python module, so we won't make a
very long example here, only a brief display of the available functions.

Let's begin by creating a "nmdb descriptor" which is of type *nmdb_t*, and
connecting it to your three servers (two TIPC at ports 11 and 12, one TCP on
localhost, default port)::

  unsigned char *key, *val;
  size_t ksize, vsize;
  nmdb_t *db;

  db = nmdb_init();
  nmdb_add_tipc_server(db, 11);
  nmdb_add_tipc_server(db, 12);
  nmdb_add_tcp_server(db, "127.0.0.1", -1);

Now you can do some operations (allocations and checks are not shown for brevity)::

  r = nmdb_set(db, key, ksize, val, vsize);
  ...
  r = nmdb_get(db, key, ksize, val, vsize);
  ...
  r = nmdb_del(db, key, ksize);

And finally close and free the connection::

  nmdb_free(db);

The operation functions have variants for cache-only (*nmdb_cache_**) and synchronous
operation (*nmdb_sync_**). For more information you should check the manpage.


Where to go from here
=====================

The best place to go from here is to your text editor, to start writing some
simple clients to play with.

If you are in doubt about something, you can consult the manpages or the
documentation inside the *doc/* directory.

If you want to report bugs, or have any questions or comments, just let me
know at albertito@blitiri.com.ar.


.. _nmdb: http://blitiri.com.ar/p/nmdb/
.. _libevent: http://www.monkey.org/~provos/libevent/
.. _TIPC: http://tipc.sf.net
.. _SCTP: http://en.wikipedia.org/wiki/Stream_Control_Transmission_Protocol
.. _memcached: http://www.danga.com/memcached/
.. _`Linux kernel`: http://kernel.org
.. _tetrations: http://en.wikipedia.org/wiki/Tetration
.. _QDBM: http://qdbm.sf.net
.. _BDB: http://www.oracle.com/technology/products/berkeley-db/db/
.. _tokyocabinet: http://1978th.net/tokyocabinet/
.. _tdb: http://tdb.samba.org


