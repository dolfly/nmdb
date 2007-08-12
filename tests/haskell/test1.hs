
-- Testing module for nmdb Haskell bindings.
-- Build with ghc --make test1.hs

module Main where

import Nmdb

-- putStrLn + show, all in one
cshow desc f = do
	r <- f
	putStr desc
	putStr " -> "
	putStrLn $ show r

main :: IO ()
main = do
	db <- nmdbInit

	cshow "Add TIPC" $ nmdbAddTIPCServer db (-1)
	cshow "Add TCP" $ nmdbAddTCPServer db "localhost" (-1)
	cshow "Add UDP" $ nmdbAddUDPServer db "localhost" (-1)

	cshow "Set 'Hello' 'Bye'" $ nmdbSet db "Hello" "Bye"

	cshow "Get 'Hello'" $ nmdbGet db "Hello"
	cshow "Get 'XYZ'" $ nmdbGet db "XYZ"

	cshow "CAS 'Hello' 'Bye' 'Hey'" $ nmdbCAS db "Hello" "Bye" "Hey"
	cshow "Get 'Hello'" $ nmdbGet db "Hello"

	cshow "Del 'Hello'" $ nmdbDel db "Hello"

	cshow "Free" $ nmdbFree db

	return ()

