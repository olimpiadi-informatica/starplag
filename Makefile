all: build/main

build/main: main.cpp file.hpp root_subs.hpp subs_dist.hpp
	mkdir -p build
	g++ -std=c++17 -O3 -Wall -Wextra main.cpp -o build/main

.PHONY: all
