all: build/compare build/main

headers := $(wildcard *.hpp)

build/compare: compare.cpp ${headers} Makefile
	mkdir -p build
	g++ -std=c++17 -g -O3 -static -Wall -Wextra compare.cpp -o build/compare

build/main: main.cpp ${headers} Makefile
	mkdir -p build
	g++ -march=native -pthread -std=c++17 -g -O3 -Wall -Wextra main.cpp -o build/main

build/pisa: main.cpp ${headers} Makefile
	mkdir -p build
	g++ -static -march=opteron-sse3 -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -std=c++17 -g -O3 -Wall -Wextra main.cpp -o build/pisa

.PHONY: all
