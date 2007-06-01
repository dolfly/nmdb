
======================
nmdb_ Network Protocol
======================
:Author: Alberto Bertogli (albertito@gmail.com)

**NOTE:** All integers are in network byte order.

The nmdb network protocol relies on a message passing underlying transport
protocol. It normally uses TIPC, but can use TCP with a messaging layer too.
This document describes the protocol in a transport-independent way, assuming
the transport protocol can send and receive messages reliably and preserve
message boundaries. No ordering guarantees are required.


Requests
========

All requests begin with a common header, and then have a request-specific
payload. They look like this::

  +-----+------------+------------------+--- - - - ---+
  | Ver | Request ID |   Request code   |   Payload   |
  +-----+------------+------------------+--- - - - ---+

Where the fields are:

Ver
  Version of the protocol. 4 bits. Must be 1.
Request ID
  A number identifying the request. A request is uniquely represented by the
  *(ID, sender)* pair, where *sender* is the sender host information. IDs can
  be reused once a matching response has arrived to the sender. 28 bits.
Request code
  The code of the operation to be performed. They're defined in the server
  source code. 32 bits.
Payload
  The payload is specific to the request code. Some requests can carry no
  associated payload.


Request codes
-------------

The following table was taken from the server source, which should be the
authoritative source of information. The codes are included here just for
completeness.

============== ======
     Name       Code
============== ======
REQ_CACHE_GET  0x101
REQ_CACHE_SET  0x102
REQ_CACHE_DEL  0x103
REQ_GET        0x104
REQ_SET_SYNC   0x105
REQ_DEL_SYNC   0x106
REQ_SET_ASYNC  0x107
REQ_DEL_ASYNC  0x108
REQ_CACHE_CAS  0x109
REQ__CAS       0x110
============== ======


Request payload formats
-----------------------

REQ_GET and REQ_CACHE_GET
  These requests have the same payload format, and only differ on the code.
  First the key size (32 bits), and then the key.
REQ_SET_* and REQ_CACHE_SET
  Like the previous requests, they share the payload format. First the key
  size (32 bits), then the value size (32 bits), then the key, and then the
  value.
REQ_DEL_* and REQ_CACHE_DEL
  You guessed it, they share the payload format too: first the key size (32
  bits), and then the key.
REQ_CAS and REQ_CACHE_CAS
  First the key size, then the old value size, then the new value size, and
  then the key, the old value and the new value.


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
  for *REQ_GET* the first 32 bits are the value size, and then the value.


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


.. _nmdb: http://auriga.wearlab.de/~alb/nmdb/

