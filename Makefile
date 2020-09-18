all: build/compare

build/compare: compare.cpp file.hpp root_subs.hpp subs_dist.hpp
	mkdir -p build
	g++ -std=c++17 -g -O3 -Wall -Wextra compare.cpp -o build/compare

.PHONY: all
