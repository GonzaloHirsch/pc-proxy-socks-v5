# Makefile
.PHONY: all sctp_client clean

#CFLAGS=-Wall -g -fsanitize=address
CFLAGS=-Wall -g
# LDFLAGS=-lrt -pthread

SOURCES=$(wildcard src/*.c)
SOURCES2=$(wildcard src/**/*.c)
INCLUDE=include

all:
	$(CC) $(CFLAGS) -g -pthread $(SOURCES) $(SOURCES2) -I$(INCLUDE) -L/usr/local/lib -lsctp -o server

client:
	$(CC) -g tests/socksClient.c tests/Utility.c -Iinclude -o socksClient
sctp_client:
	$(CC) $(CFLAGS) -g management_client/sctpClient.c management_client/sctpArgs.c tests/Utility.c -Iinclude -L/usr/local/lib -lsctp -o sctp_client
clean:
	rm -rf application slaveProcess view sctp_client socksClient

# find . -type f -exec touch {} +
# valgrind --leak-check=yes --track-origins=yes --trace-children=yes ./sctp_client
