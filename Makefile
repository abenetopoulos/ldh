.PHONY: release debug clean

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	COMP?=g++
else
	COMP?=clang++
endif

STD?=c++17
BIN?=ldh

release:
	${COMP} -I ./include --std=${STD} -DSPDLOG_COMPILED_LIB -O2 src/main.cpp \
		-Llibs/ -lgit2 -lspdlog -lpthread \
		-o ${BIN}

debug:
	${COMP} -I ./include --std=${STD} -DDEBUG src/main.cpp -g -Llibs/ -lgit2 -o ${BIN}

clean:
	rm -f ${BIN}
