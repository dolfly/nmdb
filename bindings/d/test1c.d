
import nmdb;
import std.stdio;
import std.stream;
import std.string;
import std.perf;


int main(char [][] argv)
{
	if (argv.length != 2) {
		writefln("Usage: test1d TIMES");
		return 1;
	}

	auto times = atoi(argv[1]);
	char[] val;

	nmdb.DB db = new nmdb.DB();
	db.add_tipc_server();
	db.mode = nmdb.MODE_CACHE;

	auto counter = new PerformanceCounter;

	counter.start();
	for (int i = 0; i < times; i++)
		db["1"] = "D";
	counter.stop();
	writefln("d set: ", counter.microseconds());

	counter.start();
	for (int i = 0; i < times; i++)
		val = db["1"];
	counter.stop();
	writefln("d get: ", counter.microseconds());

	counter.start();
	for (int i = 0; i < times; i++)
		db.remove("1");
	counter.stop();
	writefln("d del: ", counter.microseconds());

	return 0;
}

