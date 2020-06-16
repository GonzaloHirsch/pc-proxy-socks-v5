# Makefile

GCC=gcc
GCCFLAGS=-Wall -g -fsanitize=address
# LDFLAGS=-lrt -pthread


SOURCES=$(wildcard src/*.c)
SOURCES2=$(wildcard src/**/*.c)
INCLUDE=include

all:
	gcc -g -pthread $(SOURCES) $(SOURCES2) -I$(INCLUDE) -o server -Wall

client:
	gcc -g tests/socksClient.c tests/Utility.c -Iinclude -o socksClient -Wall
clean:
	rm -rf application slaveProcess view
