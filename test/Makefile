WORKDIR=/home/admin/shared/

CXX=g++
MPICXX=mpicxx
CXXFLAGS=-O3 -std=c++17 -g
LDFLAGS=

all: test_timestamp_format

test_timestamp_format:
	$(CXX) test_timestamp_format.cpp -o test_timestamp_format $(CXXFLAGS) $(LDFLAGS)

run:
	./test_timestamp_format

clean:
	rm -rf test_timestamp_format
