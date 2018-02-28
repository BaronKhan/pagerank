# If using openCL on edge04, make sure to run the following command -
# ln -s /usr/lib/x86_64-linux-gnu/libOpenCL.so.1 libOpenCL.so ;
CFLAGS=-O3 -std=c++11 -I ../include -I ../opencl_sdk/include
LDFLAGS=-ltbb -L /usr/lib/x86_64-linux-gnu -L . -l OpenCL

pagerank_test: pagerank_test.cpp table.cpp pagerank.cpp table.h
	g++ $(CFLAGS) -o pagerank_test pagerank_test.cpp table.cpp $(LDFLAGS)
pagerank: pagerank.cpp table.cpp table.h
	g++ $(CFLAGS) -Wall -o pagerank pagerank.cpp table.cpp $(LDFLAGS)

all-tests: all-tests.txt pagerank_test
	./pagerank_test all-tests.txt

small-test: small pagerank_test
	./pagerank_test small

medium-test: medium pagerank_test
	./pagerank_test medium

large-test: large pagerank_test
	./pagerank_test large

enormous-test: enormous pagerank_test
	./pagerank_test enormous

ginormous-test: ginormous pagerank_test
	./pagerank_test ginormous

clean:
	rm -f pagerank pagerank_test
