
import nmdb;
import std.stdio;
import std.stream;


int main()
{
	char[] val1;

	nmdb.DB db = new nmdb.DB();
	db.add_tipc_server();

	db.mode = MODE_CACHE;
	db["1"] = "D";
	val1 = db["1"];

	writefln(val1);

	return 0;
}

