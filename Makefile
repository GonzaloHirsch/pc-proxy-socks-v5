GCC=gcc
GCCFLAGS=-Wall -g
# LDFLAGS=-lrt -pthread

SOURCES=$(wildcard src/*.c)
INCLUDE=include

all:
	gcc -g $(SOURCES) -I$(INCLUDE) -o server

clean:
	rm -rf application slaveProcess view
