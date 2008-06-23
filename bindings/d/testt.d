
import nmdb;
import std.stdio;
import std.stream;


int main()
{
	char[] val1;
	long newval, ret;

	nmdb.DB db = new nmdb.DB();
	db.add_tipc_server();

	db.mode = MODE_CACHE;
	db["D"] = "1\0";
	val1 = db["D"];

	writefln(val1);

	ret = db.incr("D", 4, &newval);
	writefln(newval, " ", db["D"]);

	return 0;
}

