
;; Bigloo nmdb bindings

(module nmdb

	;; C functions
	(extern
	  (type _nmdb_t (pointer void) "void *")

	  (macro _nmdb_init::_nmdb_t () "nmdb_init")
	  (macro _nmdb_free::int (::_nmdb_t) "nmdb_free")

	  (macro _nmdb_add_tipc_server::int (::_nmdb_t ::int)
		 "nmdb_add_tipc_server")
	  (macro _nmdb_add_tcp_server::int (::_nmdb_t ::string ::int)
		 "nmdb_add_tcp_server")
	  (macro _nmdb_add_udp_server::int (::_nmdb_t ::string ::int)
		 "nmdb_add_udp_server")
	  (macro _nmdb_add_sctp_server::int (::_nmdb_t ::string ::int)
		 "nmdb_add_sctp_server")

	  (macro _nmdb_set::int
		 (::_nmdb_t ::string ::uint ::string ::uint)
		 "nmdb_set")
	  (macro _nmdb_set_sync::int
		 (::_nmdb_t ::string ::uint ::string ::uint)
		 "nmdb_set_sync")
	  (macro _nmdb_cache_set::int
		 (::_nmdb_t ::string ::uint ::string ::uint)
		 "nmdb_cache_set")

	  (macro _nmdb_get::ulong
		 (::_nmdb_t ::string ::uint ::string ::uint)
		 "nmdb_get")
	  (macro _nmdb_cache_get::ulong
		 (::_nmdb_t ::string ::uint ::string ::uint)
		 "nmdb_cache_get")

	  (macro _nmdb_del::int
		 (::_nmdb_t ::string ::uint)
		 "nmdb_del")
	  (macro _nmdb_del_sync::int
		 (::_nmdb_t ::string ::uint)
		 "nmdb_del_sync")
	  (macro _nmdb_cache_del::int
		 (::_nmdb_t ::string ::uint)
		 "nmdb_cache_del")

	  (macro _nmdb_cas::int
		 (::_nmdb_t ::string ::uint ::string ::uint ::string ::uint)
		 "nmdb_cas")
	  (macro _nmdb_cache_cas::int
		 (::_nmdb_t ::string ::uint ::string ::uint ::string ::uint)
		 "nmdb_cache_cas")

	  (macro _nmdb_incr::ulong
		 (::_nmdb_t ::string ::uint ::ulong)
		 "nmdb_incr")
	  (macro _nmdb_cache_incr::ulong
		 (::_nmdb_t ::string ::uint ::ulong)
		 "nmdb_cache_incr")

	  )

	(export
	  (make-nmdb)
	  (nmdb-free db)

	  (nmdb-add-tipc-server db port)
	  (nmdb-add-tcp-server db addr port)
	  (nmdb-add-udp-server db addr port)
	  (nmdb-add-sctp-server db addr port)

	  (nmdb-get db key)
	  (nmdb-cache-get db key)

	  (nmdb-set db key val)
	  (nmdb-set-sync db key val)
	  (nmdb-cache-set db key val)

	  (nmdb-del db key)
	  (nmdb-del-sync db key)
	  (nmdb-cache-del db key)

	  (nmdb-cas db key oldval newval)
	  (nmdb-cache-cas db key oldval newval)

	  (nmdb-incr db key increment)
	  (nmdb-cache-incr db key increment)
	  )

	)


;; creator and destructor
(define (make-nmdb) (_nmdb_init))
(define (nmdb-free db) (_nmdb_free db))

;; adding servers
(define (nmdb-add-tipc-server db port) (_nmdb_add_tipc_server db port))
(define (nmdb-add-tcp-server db addr port) (_nmdb_add_tcp_server db addr port))
(define (nmdb-add-udp-server db addr port) (_nmdb_add_udp_server db addr port))
(define (nmdb-add-sctp-server db addr port) (_nmdb_add_sctp_server db addr port))

;; get functions
(define (nmdb-generic-get func db key)
  (define buflen (* 70 1024))
  (define buf (make-string buflen))
  (define vsize (func db key (string-length key) buf buflen))
  (if (< vsize 0)
    vsize
    (substring buf 0 vsize) )
  )

(define (nmdb-get db key) (nmdb-generic-get _nmdb_get db key))
(define (nmdb-cache-get db key) (nmdb-generic-get _nmdb_cache_get db key))

;; set functions
(define (nmdb-generic-set func db key val)
  (func db key (string-length key) val (string-length val)) )
(define (nmdb-set db key val)
  (nmdb-generic-set _nmdb_set db key val))
(define (nmdb-set-sync db key val)
  (nmdb-generic-set _nmdb_set_sync db key val))
(define (nmdb-cache-set db key val)
  (nmdb-generic-set _nmdb_cache_set db key val))

;; del functions
(define (nmdb-generic-del func db key)
  (func db key (string-length key)) )
(define (nmdb-del db key)
  (nmdb-generic-del _nmdb_del db key))
(define (nmdb-del-sync db key)
  (nmdb-generic-del _nmdb_del_sync db key))
(define (nmdb-cache-del db key)
  (nmdb-generic-del _nmdb_cache_del db key))

;; cas functions
(define (nmdb-generic-cas func db key oldval newval)
  (func db key (string-length key)
	oldval (string-length oldval)
	newval (string-length newval) ) )
(define (nmdb-cas db key oldval newval)
  (nmdb-generic-cas _nmdb_cas db key oldval newval))
(define (nmdb-cache-cas db key oldval newval)
  (nmdb-generic-cas _nmdb_cache_cas db key oldval newval))

;; incr functions
(define (nmdb-generic-incr func db key increment)
  (func db key (string-length key) increment ) )
(define (nmdb-incr db key increment)
  (nmdb-generic-incr _nmdb_incr db key increment))
(define (nmdb-cache-incr db key increment)
  (nmdb-generic-incr _nmdb_cache_incr db key increment))

