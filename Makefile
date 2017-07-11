CXX=clang++
BUILD_DIR=build
BUILD_TYPE=Debug

all: build


build:
	mkdir -p ${BUILD_DIR} && \
	cd ${BUILD_DIR} && \
	cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CXX} .. && \
	cmake --build .


clean:
	rm -fr ${BUILD_DIR}


format:
	clang-format -i vadem.cc


${BUILD}/vadem: 
	${CXX} -o vadem vadem.cc ${CXXFLAGS} ${LDFLAGS}


.PHONY: all build clean format
