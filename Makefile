CXXFLAGS=-g -Wall -Wextra -std=c++11 -isystem png++
LDFLAGS=-lva -lva-drm -lpng

all: vadem


clean:
	rm -f vadem


format:
	clang-format -i vadem.cc


vadem: vadem.cc Makefile
	g++ -o vadem vadem.cc ${CXXFLAGS} ${LDFLAGS}


.PHONY: all clean format
