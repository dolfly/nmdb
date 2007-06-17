
#ifndef _TEST_PROTOTYPES_H
#define _TEST_PROTOTYPES_H


#if USE_NORMAL
  #define NGET(...) nmdb_get(__VA_ARGS__)
  #define NSET(...) nmdb_set(__VA_ARGS__)
  #define NDEL(...) nmdb_del(__VA_ARGS__)
  #define NCAS(...) nmdb_cas(__VA_ARGS__)
#elif USE_CACHE
  #define NGET(...) nmdb_cache_get(__VA_ARGS__)
  #define NSET(...) nmdb_cache_set(__VA_ARGS__)
  #define NDEL(...) nmdb_cache_del(__VA_ARGS__)
  #define NCAS(...) nmdb_cache_cas(__VA_ARGS__)
#elif USE_SYNC
  #define NGET(...) nmdb_get(__VA_ARGS__)
  #define NSET(...) nmdb_set_sync(__VA_ARGS__)
  #define NDEL(...) nmdb_del_sync(__VA_ARGS__)
  #define NCAS(...) nmdb_cas(__VA_ARGS__)
#endif


#if USE_TCP
  #define NADDSRV(db) nmdb_add_tcp_server(db, "localhost", -1)
#elif USE_UDP
  #define NADDSRV(db) nmdb_add_udp_server(db, "localhost", -1)
#elif USE_TIPC
  #define NADDSRV(db) nmdb_add_tipc_server(db, -1)
#elif USE_MULT
  #define NADDSRV(db) \
	do { \
		nmdb_add_tipc_server(db, -1); \
		nmdb_add_tcp_server(db, "localhost", -1); \
		nmdb_add_udp_server(db, "localhost", -1); \
	} while (0)
#endif


#endif

