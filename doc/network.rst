
======================
nmdb_ Network Protocol
======================
:Author: Alberto Bertogli (albertito@blitiri.com.ar)

**NOTE:** All integers are in network byte order.

The nmdb network protocol relies on a message passing underlying transport
protocol. It can be used on top of TIPC, UDP, TCP (with a thin messaging
layer) or SCTP. This document describes the protocol in a
transport-independent way, assuming the transport protocol can send and
receive messages reliably and preserve message boundaries. No ordering
guarantee is required for the request-reply part of the protocol, but it is
highly desirable to avoid reordering of requests.


Requests
========

All requests begin with a common header, and then have a request-specific
payload. They look like this::

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |    Version    |                 Request ID                    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |         Request code          |             Flags             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  :                            Payload                            :
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


Where the fields are:

Ver (4 bits)
  Version of the protocol. Must be 1.
Request ID (28 bits)
  A number identifying the request. A request is uniquely represented by the
  *(ID, sender)* pair, where *sender* is the sender host information. IDs can
  be reused once a matching response has arrived to the sender.
Request code (16 bits)
  The code of the operation to be performed. They're defined in the server
  source code.
Flags (16 bits)
  Flags that affect the request code.
Payload (variable, can be absent)
  The payload is specific to the request code. Some requests carry no
  associated payload.


Request codes
-------------

The following table was taken from the server source, which should be the
authoritative source of information. The codes are included here just for
completeness.

============== ======
     Name       Code
============== ======
REQ_GET        0x101
REQ_SET        0x102
REQ_DEL        0x103
REQ_CAS        0x104
REQ_INCR       0x105
============== ======


Flags
-----

Note that not all requests accept all the flags. Flags that are not relevant
for a given request will be ignored.

================= ====== =============================================
      Name         Code                   Relevant to
================= ====== =============================================
FLAGS_CACHE_ONLY      1  REQ_GET, REQ_SET, REQ_DEL, REQ_CAS, REQ_INCR
FLAGS_SYNC            2  REQ_SET, REQ_DEL
================= ====== =============================================


Request payload formats
-----------------------

REQ_GET
  These requests have the same payload format, and only differ on the code.
  First the key size (32 bits), and then the key.
REQ_SET
  Like the previous requests, they share the payload format. First the key
  size (32 bits), then the value size (32 bits), then the key, and then the
  value.
REQ_DEL
  You guessed it, they share the payload format too: first the key size (32
  bits), and then the key.
REQ_CAS
  First the key size, then the old value size, then the new value size, and
  then the key, the old value and the new value.
REQ_INCR
  First the key size (32 bits), then the key, and then the increment as a
  signed network byte order 64 bit integer.


Replies
=======

Replies begin with the ID they correspond to, then a reply code, and then a
reply-specific payload. They look like this::

  +------------+------------------+--- - - - ---+
  | Request ID |    Reply code    |   Payload   |
  +------------+------------------+--- - - - ---+

Where the fields are:

Request ID
  The request ID this reply corresponds to. 32 bits.
Reply code
  The code of the reply. They're defined in the server
Payload
  The payload is specific to the reply code. Some replies can carry no
  associated payload.

All integers are in network byte ordering.


Reply codes
-----------

The following table was taken from the server source, which should be the
authoritative source of information. The codes are included here just for
completeness.

================ ======
      Name        Code
================ ======
REP_ERR          0x800
REP_CACHE_HIT    0x801
REP_CACHE_MISS   0x802
REP_OK           0x803
REP_NOTIN        0x804
REP_NOMATCH      0x805
================ ======


Reply payload formats
---------------------

REP_ERR
  The payload is a 32-bit error code, according to the table below.
REP_CACHE_MISS, REP_NOTIN and REP_NOMATCH
  These replies have no payload.
REP_CACHE_HIT
  The first 32 bits are the value size, then the value.
REP_OK
  Depending on the request, this reply does or doesn't have an associated
  value. For *REQ_SET**, *REQ_DEL** and *REQ_CAS** there is no payload. But
  for *REQ_GET* the first 32 bits are the value size, and then the value; and
  for *REQ_INCR* the first 32 bits are the payload size, and then the
  post-increment value as a signed 64-bit integer in network byte order.


Reply error codes
-----------------

============ ====== =========================
    Name      Code         Description
============ ====== =========================
ERR_VER      0x101  Version mismatch
ERR_SEND     0x102  Error sending data
ERR_BROKEN   0x103  Broken request
ERR_UNKREQ   0x104  Unknown request
ERR_MEM      0x105  Memory allocation error
ERR_DB       0x106  Database error
============ ====== =========================


.. _nmdb: http://blitiri.com.ar/p/nmdb/

