all: build/compare build/main

build/compare: compare.cpp file.hpp root_subs.hpp subs_dist.hpp Makefile
	mkdir -p build
	g++ -std=c++17 -g -O3 -Wall -Wextra compare.cpp -o build/compare

build/main: main.cpp file.hpp root_subs.hpp subs_dist.hpp Makefile
	mkdir -p build
	g++ -ltcmalloc -march=native -pthread -std=c++17 -g -O3 -Wall -Wextra main.cpp -o build/main

.PHONY: all
