#!/usr/bin/env ruby

require 'mkmf'

if have_library("nmdb", "nmdb_init") and have_header("nmdb.h")
then
	dir_config("libnmdb")

	create_makefile("nmdb_ll")
else
	puts "Can't find libnmdb"
end

