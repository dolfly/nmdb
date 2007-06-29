
module Nmdb

require "nmdb_ll"


class NetworkException < StandardError
end


class GenericDB
	attr_accessor :automarshal
	attr_accessor :default

	def initialize(default = nil)
		@db = Nmdb_ll::DB.new
		@automarshal = true
		@default = default
	end

	def add_tipc_server(port = -1)
		return @db.add_tipc_server(port)
	end

	def add_tcp_server(host, port = 26010)
		return @db.add_tcp_server(port)
	end

	def add_udp_server(host, port = 26010)
		return @db.add_udp_server(port)
	end


	def normal_set(key, val)
		if @automarshal then
			key = Marshal.dump(key)
			val = Marshal.dump(val)
		end
		return @db.set(key, val)
	end

	def set_sync(key, val)
		if @automarshal then
			key = Marshal.dump(key)
			val = Marshal.dump(val)
		end
		return @db.set_sync(key, val)
	end

	def cache_set(key, val)
		if @automarshal then
			key = Marshal.dump(key)
			val = Marshal.dump(val)
		end
		return @db.cache_set(key, val)
	end


	def generic_get(gfunc, key)
		if @automarshal then
			key = Marshal.dump(key)
		end
		r = gfunc.call(key)
		if r.class == String then
			if @automarshal then
				r = Marshal.load(r)
			end
			return r
		elsif r == -1 then
			# key not in the database
			return @default
		elsif r <= 2 then
			raise NetworkException
		else
			# we should never get here
			raise Exception
		end
	end

	def normal_get(key)
		return generic_get(@db.method(:get), key)
	end

	def cache_get(key)
		return generic_get(@db.method(:cache_get), key)
	end


	def normal_delete(key)
		if @automarshal then
			key = Marshal.dump(key)
		end
		return @db.del(key)
	end

	def cache_delete(key)
		if @automarshal then
			key = Marshal.dump(key)
		end
		return @db.cache_del(key)
	end

	def delete_sync(key)
		if @automarshal then
			key = Marshal.dump(key)
		end
		return @db.del_sync(key)
	end


	def generic_cas(gfunc, key, old, new)
		if @automarshal then
			key = Marshal.dump(key)
			old = Marshal.dump(old)
			new = Marshal.dump(new)
		end
		r = gfunc.call(key, old, new)
		if r == 0 then
			# key not in the database
			return nil
		elsif r < 0 then
			raise NetworkException
		else
			return r
		end
	end

	def normal_cas(key, old, new)
		return generic_cas(@db.method(:cas), key, old, new)
	end

	def cache_cas(key, old, new)
		return generic_cas(@db.method(:cache_cas), key, old, new)
	end


	# The following functions asume we have set(), get(), delete() and
	# cas(), which are supposed to be implemented by our subclasses

	def include?(key)
		# we assume we have get() implemented
		return get(key) == @default
	end

	def []=(key, val)
		return set(key, val)
	end

	def [](key)
		return get(key)
	end
end


class DB < GenericDB
	alias set normal_set
	alias get normal_get
	alias delete normal_delete
	alias cas normal_cas
end

class Cache < GenericDB
	alias set cache_set
	alias get cache_get
	alias delete cache_delete
	alias cas cache_cas
end

class Sync < GenericDB
	alias set set_sync
	alias get normal_get
	alias delete delete_sync
	alias cas normal_cas
end

end

