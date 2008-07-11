
-- Haskell bindings for the nmdb C library
-- Alberto Bertogli (albertito@blitiri.com.ar)

module Nmdb (
	NmdbStruct,
	nmdbInit, nmdbFree,
	nmdbAddTIPCServer, nmdbAddTCPServer, nmdbAddUDPServer,
		nmdbAddSCTPServer,
	nmdbSet, nmdbSetSync, nmdbCacheSet,
	nmdbGet, nmdbCacheGet,
	nmdbDel, nmdbDelSync, nmdbCacheDel,
	nmdbCAS, nmdbCacheCAS,
	nmdbIncr, nmdbCacheIncr,
) where

import Foreign
import Foreign.Ptr
import Foreign.Storable
import Foreign.C.Types
import Foreign.C.String
import Foreign.Marshal.Alloc


-- Opaque pointer to nmdb_t
data NmdbStruct = NmdbStruct
type NmdbPtr = Ptr NmdbStruct


-- DB creation and destroy
foreign import ccall "nmdb.h nmdb_init" llNmdbInit :: IO NmdbPtr
foreign import ccall "nmdb.h nmdb_free" llNmdbFree :: NmdbPtr -> IO ()

nmdbInit :: IO NmdbPtr
nmdbInit = llNmdbInit

nmdbFree :: NmdbPtr -> IO ()
nmdbFree = llNmdbFree


-- Adding servers
foreign import ccall "nmdb.h nmdb_add_tipc_server" llNmdbAddTIPCServer ::
	NmdbPtr -> Int -> IO Int
foreign import ccall "nmdb.h nmdb_add_tcp_server" llNmdbAddTCPServer ::
	NmdbPtr -> CString -> Int -> IO Int
foreign import ccall "nmdb.h nmdb_add_udp_server" llNmdbAddUDPServer ::
	NmdbPtr -> CString -> Int -> IO Int
foreign import ccall "nmdb.h nmdb_add_sctp_server" llNmdbAddSCTPServer ::
	NmdbPtr -> CString -> Int -> IO Int

nmdbAddTIPCServer db port = do
	r <- llNmdbAddTIPCServer db port
	return r

nmdbAddTCPServer db host port = do
	hstr <- newCString host
	r <- llNmdbAddTCPServer db hstr port
	free hstr
	return r

nmdbAddUDPServer db host port = do
	hstr <- newCString host
	r <- llNmdbAddUDPServer db hstr port
	free hstr
	return r

nmdbAddSCTPServer db host port = do
	hstr <- newCString host
	r <- llNmdbAddSCTPServer db hstr port
	free hstr
	return r

-- Set functions
foreign import ccall "nmdb.h nmdb_set" llNmdbSet ::
	NmdbPtr -> CString -> Int -> CString -> Int -> IO Int
foreign import ccall "nmdb.h nmdb_set_sync" llNmdbSetSync ::
	NmdbPtr -> CString -> Int -> CString -> Int -> IO Int
foreign import ccall "nmdb.h nmdb_cache_set" llNmdbCacheSet ::
	NmdbPtr -> CString -> Int -> CString -> Int -> IO Int

nmdbGenericSet llfunc db key val = do
	kl <- newCStringLen key
	vl <- newCStringLen val
	r <- llfunc db (fst kl) (snd kl) (fst vl) (snd vl)
	free (fst kl)
	free (fst vl)
	return r

nmdbSet = nmdbGenericSet llNmdbSet
nmdbSetSync = nmdbGenericSet llNmdbSetSync
nmdbCacheSet = nmdbGenericSet llNmdbCacheSet


-- Get functions
foreign import ccall "nmdb.h nmdb_get" llNmdbGet ::
	NmdbPtr -> CString -> Int -> CString -> Int -> IO Int
foreign import ccall "nmdb.h nmdb_cache_get" llNmdbCacheGet ::
	NmdbPtr -> CString -> Int -> CString -> Int -> IO Int

nmdbGenericGet llfunc db key = do
	let buflen = 64 * 1024
	buf <- mallocBytes buflen
	kl <- newCStringLen key
	r <- llfunc db (fst kl) (snd kl) buf buflen
	free (fst kl)
	if r < 0
	  then do
		free buf
		return Nothing
	  else do
		val <- peekCStringLen (buf, r)
		free buf
		return $ Just val

nmdbGet = nmdbGenericGet llNmdbGet
nmdbCacheGet = nmdbGenericGet llNmdbCacheGet


-- Del functions
foreign import ccall "nmdb.h nmdb_del" llNmdbDel ::
	NmdbPtr -> CString -> Int -> IO Int
foreign import ccall "nmdb.h nmdb_del_sync" llNmdbDelSync ::
	NmdbPtr -> CString -> Int -> IO Int
foreign import ccall "nmdb.h nmdb_cache_del" llNmdbCacheDel ::
	NmdbPtr -> CString -> Int -> IO Int

nmdbGenericDel llfunc db key = do
	kl <- newCStringLen key
	r <- llfunc db (fst kl) (snd kl)
	free (fst kl)
	return r

nmdbDel = nmdbGenericDel llNmdbDel
nmdbDelSync = nmdbGenericDel llNmdbDelSync
nmdbCacheDel = nmdbGenericDel llNmdbCacheDel


-- CAS functions
foreign import ccall "nmdb.h nmdb_cas" llNmdbCAS ::
	NmdbPtr -> CString -> Int -> CString -> Int -> CString -> Int -> IO Int
foreign import ccall "nmdb.h nmdb_cache_cas" llNmdbCacheCAS ::
	NmdbPtr -> CString -> Int -> CString -> Int -> CString -> Int -> IO Int

nmdbGenericCAS llfunc db key oldval newval = do
	kl <- newCStringLen key
	ovl <- newCStringLen oldval
	nvl <- newCStringLen newval
	r <- llfunc db
		(fst kl) (snd kl)
		(fst ovl) (snd ovl)
		(fst nvl) (snd nvl)
	free (fst kl)
	free (fst ovl)
	free (fst nvl)
	return r

nmdbCAS = nmdbGenericCAS llNmdbCAS
nmdbCacheCAS = nmdbGenericCAS llNmdbCacheCAS


-- Incr functions
foreign import ccall "nmdb.h nmdb_incr" llNmdbIncr ::
	NmdbPtr -> CString -> Int -> Int64 -> Ptr Int64 -> IO Int
foreign import ccall "nmdb.h nmdb_cache_incr" llNmdbCacheIncr ::
	NmdbPtr -> CString -> Int -> Int64 -> Ptr Int64 -> IO Int

nmdbGenericIncr llfunc db key increment = do
	kl <- newCStringLen key
	newval <- malloc
	r <- llfunc db (fst kl) (snd kl) increment newval
	v <- (peek newval)
	free (fst kl)
	free newval
	if r == 2
	  then do
		return $ Just v
	  else
		return Nothing

nmdbIncr = nmdbGenericIncr llNmdbIncr
nmdbCacheIncr = nmdbGenericIncr llNmdbCacheIncr


