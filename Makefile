GCC=gcc
GCCFLAGS=-Wall -g
# LDFLAGS=-lrt -pthread


SOURCES=$(wildcard src/*.c)
SOURCES2=$(wildcard src/**/*.c)
INCLUDE=include

all:
	gcc -g -pthread $(SOURCES) $(SOURCES2) -I$(INCLUDE) -o server

clean:
	rm -rf application slaveProcess view
