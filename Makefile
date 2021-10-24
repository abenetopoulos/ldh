.PHONY: all clean

STD?=c++17
COMP?=g++
BIN?=ldh

all:
	${COMP} -I ./include --std=${STD} src/main.cpp -g -o ${BIN}

clean:
	rm -f ${BIN}
