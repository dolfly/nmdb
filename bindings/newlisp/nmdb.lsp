
;
; newlisp (http://www.newlisp.org/) bindings for nmdb
; Alberto Bertogli (albertito@gmail.com)
;
; Functions:
;
;   (nmdb:init) -> Creates a new database object.
;   (nmdb:add-tipc-server port) -> Adds a new TIPC server to the database.
;   (nmdb:add-tcp-server addr port) -> Adds a new TCP server to the database.
;   (nmdb:add-udp-server addr port) -> Adds a new UDP server to the database.
;   (nmdb:free) -> Closes the database.
;
;   (nmdb:db-get key) -> Gets the value associated to the given key, or -1.
;   (nmdb:cache-get key) -> Like dbget but only get from the cache.
;
;   (nmdb:db-set key val) -> Sets the given key to the given value.
;   (nmdb:sync-set key val) -> Like db-set but synchronous.
;   (nmdb:cache-set key val) -> Like db-set but only set to the the cache.
;
;   (nmdb:db-del key) -> Removes the given key from the database.
;   (nmdb:sync-del key) -> Like db-del but synchronous.
;   (nmdb:cache-del key) -> Like db-del but only delete from the cache.
;
;
; Example:
;   (load "nmdb.lsp")
;   (nmdb:init)
;   (nmdb:add-tipc-server 10)
;   (nmdb:db-set "Hello" "Newlisp!")
;   (nmdb:db-get "Hello")
;   (nmdb:db-del "Hello")
;   (nmdb:free)
;
; For more information check the nmdb docs.
;


(context 'nmdb)

; library loading
(set 'libnmdb "libnmdb.so")

(import libnmdb "nmdb_init")
(import libnmdb "nmdb_add_tipc_server")
(import libnmdb "nmdb_add_tcp_server")
(import libnmdb "nmdb_add_udp_server")
(import libnmdb "nmdb_add_sctp_server")
(import libnmdb "nmdb_free")

(import libnmdb "nmdb_set")
(import libnmdb "nmdb_set_sync")
(import libnmdb "nmdb_cache_set")

(import libnmdb "nmdb_get")
(import libnmdb "nmdb_cache_get")

(import libnmdb "nmdb_del")
(import libnmdb "nmdb_del_sync")
(import libnmdb "nmdb_cache_del")

(import libnmdb "nmdb_cas")
(import libnmdb "nmdb_cache_cas")

(import libnmdb "nmdb_incr")
(import libnmdb "nmdb_cache_incr")


; main functions

(define (init port)
  (set 'NMDB (nmdb_init port))
  (if (= NMDB 0) (set NMDB nil))
  (not (= NMDB nil)))

(define (add-tipc-server port)
  (nmdb_add_tipc_server NMDB port))

(define (add-tcp-server addr port)
  (nmdb_add_tcp_server NMDB addr port))

(define (add-udp-server addr port)
  (nmdb_add_udp_server NMDB addr port))

(define (add-sctp-server addr port)
  (nmdb_add_sctp_server NMDB addr port))

(define (free)
  (nmdb_free NMDB))


; *-get functions

(define (priv-get func key)
  (letn ( (keylen (length key))
	  (vallen (* 64 1024))
	  (val (dup "\000" vallen))
	)
    (set 'rv (func NMDB key keylen val vallen))
    (if (>= rv 0)
      (slice val 0 rv)
      -1) ) )

(define (db-get key) (priv-get nmdb_get key))
(define (cache-get key) (priv-get nmdb_cache_get key))


; *-set functions

(define (priv-set func key val)
  (letn ( (keylen (length key))
	  (vallen (length val))
	)
    (func NMDB key keylen val vallen) ) )

(define (db-set key val) (priv-set nmdb_set key val))
(define (sync-set key val) (priv-set nmdb_set_sync key val))
(define (cache-set key val) (priv-set nmdb_cache_set key val))


; *-del functions
(define (priv-del func key)
  (letn ( (keylen (length key)) )
    (func NMDB key keylen) ) )

(define (db-del key) (priv-del nmdb_del key))
(define (sync-del key) (priv-del nmdb_del_sync key))
(define (cache-del key) (priv-del nmdb_cache_del key))


; *-cas functions
(define (priv-cas func key oldval newval)
  (letn ( (keylen (length key))
	  (ovlen (length oldval))
	  (nvlen (length newval))
	)
    (func NMDB key keylen oldval ovlen newval nvlen) ) )

(define (db-cas key oval nval) (priv-cas nmdb_cas key oval nval))
(define (cache-cas key oval nval) (priv-cas nmdb_cache_cas key oval nval))


; *-incr functions
(define (priv-incr func key increment)
  (letn ( (keylen (length key)) )
    (func NMDB key keylen increment) ) )

(define (db-incr key increment) (priv-incr nmdb_incr key increment))
(define (cache-incr key increment) (priv-incr nmdb_cache_incr key increment))


(context MAIN)



