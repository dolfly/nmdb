
(module test1
	(import (nmdb "nmdb.scm")) )

(define db (make-nmdb))
(nmdb-add-tipc-server db -1)

(print)
(print "db-set D1 V1\t"		(nmdb-set db "D1" "D1"))
(print "sync-set S2 V2\t"	(nmdb-set-sync db "S2" "V2"))
(print "cache-set C3 C3\t"	(nmdb-cache-set db "C3" "C3"))
(print)
(print "db-get D1\t"		(nmdb-get db "D1"))
(print "db-get S2\t"		(nmdb-get db "S2"))
(print "cache-get C3\t"		(nmdb-cache-get db "C3"))
(print)
(print "db-cas D1\t"		(nmdb-cas db "D1" "D1" "DX"))
(print "cache-cas C3\t"		(nmdb-cache-cas db "C3" "C3" "CX"))
(print)
(print "db-del D1\t"		(nmdb-del db "D1"))
(print "sync-del S2\t"		(nmdb-del-sync db "S2"))
(print "cache-del C3\t"		(nmdb-cache-del db "C3"))

(nmdb-free db)

