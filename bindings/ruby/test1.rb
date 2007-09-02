#!/usr/bin/env ruby

require 'nmdb'

db = Nmdb::DB.new
db.add_tipc_server()
db.add_udp_server('localhost')
db.add_tcp_server('localhost')
db.add_sctp_server('localhost')

puts 'set 1 <- 2'; db[1] = 2
puts 'get 1: ' + db[1].to_s

db.automarshal = false
puts 'set I <- 10\0'; db['I'] = "10\0"
puts 'incr I 10: ' + db.incr('I', 10).to_s
puts 'get I: ' + db['I'].to_s

