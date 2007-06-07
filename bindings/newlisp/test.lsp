
; To run: newlisp test.lsp

(load "nmdb.lsp")

(println "init\t\t"		(nmdb:init))
(println "add-tipc-server\t"	(nmdb:add-tipc-server 10))
;(println "add-tipc-server\t"	(nmdb:add-tipc-server 11))
;(println "add-tipc-server\t"	(nmdb:add-tipc-server 12))
;(println "add-tcp-server\t"	(nmdb:add-tcp-server "127.0.0.1" -1))
;(println "add-udp-server\t"	(nmdb:add-udp-server "127.0.0.1" -1))
(println)
(println "db-set D1 V1\t"	(nmdb:db-set "D1" "D1"))
(println "sync-set S2 V2\t"	(nmdb:sync-set "S2" "V2"))
(println "cache-set C3 C3\t"	(nmdb:cache-set "C3" "C3"))
(println)
(println "db-get D1\t"		(nmdb:db-get "D1"))
(println "db-get S2\t"		(nmdb:db-get "S2"))
(println "cache-get C3\t"	(nmdb:cache-get "C3"))
(println)
(println "db-cas D1\t"		(nmdb:db-cas "D1" "D1" "DX"))
(println "cache-cas C3\t"	(nmdb:cache-cas "C3" "C3" "CX"))
(println)
(println "db-del D1\t"		(nmdb:db-del "D1"))
(println "sync-del S2\t"	(nmdb:sync-del "S2"))
(println "cache-del C3\t"	(nmdb:cache-del "C3"))

(exit)

