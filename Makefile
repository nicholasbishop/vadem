CXXFLAGS=-g -Wall -Wextra -std=c++11 -isystem png++
LDFLAGS=-lva -lva-drm -lpng

vadem: vadem.cc Makefile
	g++ -o vadem vadem.cc ${CXXFLAGS} ${LDFLAGS}
