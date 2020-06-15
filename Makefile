# Makefile

GCC=gcc
GCCFLAGS=-Wall -g -fsanitize=address
# LDFLAGS=-lrt -pthread


SOURCES=$(wildcard src/*.c)
SOURCES2=$(wildcard src/**/*.c)
INCLUDE=include

all:
	gcc -g -pthread $(SOURCES) $(SOURCES2) -I$(INCLUDE) -L/usr/local/lib -lsctp -o server

client:
	gcc -g tests/socksClient.c tests/Utility.c -Iinclude -o socksClient
sctp_client:
	gcc -g tests/sctpClient.c -Iinclude -L/usr/local/lib -lsctp -o sctp_client
clean:
	rm -rf application slaveProcess view sctp_client socksClient server
