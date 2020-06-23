# Makefile
.PHONY: clean

#CFLAGS=-Wall -g -fsanitize=address -std=c11 -D_POSIX_C_SOURCE=200112L
CFLAGS=-Wall -g -std=c11 -D_POSIX_C_SOURCE=200112L
# LDFLAGS=-lrt -pthread

SOURCES=$(wildcard src/*.c)
SOURCES2=$(wildcard src/**/*.c)
INCLUDE=include
MGMT_INCLUDE=management_client
# HEADERS=$(wildcard include/*.h)
# HEADERS2=$(wildcard include/**/*.h)
MGMT_SOURCES=$(wildcard management_client/*.c)


all: server sctp_client

server: $(SOURCES) $(SOURCES2)
	$(CC) $(CFLAGS) -g -pthread $(SOURCES) $(SOURCES2) -I$(INCLUDE) -L/usr/local/lib -lsctp -o $@
client: 
	$(CC) -g tests/socksClient.c tests/Utility.c -Iinclude -o socksClient
sctp_client: $(MGMT_SOURCES)
	$(CC) $(CFLAGS) -g $(MGMT_SOURCES) -I$(INCLUDE) -I$(MGMT_INCLUDE) -Itests/ -L/usr/local/lib -lsctp -o $@
clean:
	rm -rf application slaveProcess view sctp_client socksClient server
