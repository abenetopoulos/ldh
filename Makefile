.PHONY: all clean

STD?=c++17
COMP?=g++
BIN?=ldh

release:
	${COMP} -I ./include --std=${STD} src/main.cpp -o ${BIN}

debug:
	${COMP} -I ./include --std=${STD} -DDEBUG src/main.cpp -g -Llibs/ -lgit2 -o ${BIN}

clean:
	rm -f ${BIN}
