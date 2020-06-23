# Makefile
.PHONY: clean

CFLAGS=-Wall -g -fsanitize=address -std=c11 -D_POSIX_C_SOURCE=200112L
#CFLAGS=-Wall -g -std=c11 -D_POSIX_C_SOURCE=200112L
# LDFLAGS=-lrt -pthread

SOURCES=$(wildcard src/*.c)
SOURCES2=$(wildcard src/**/*.c)
INCLUDE=include
MGMT_INCLUDE=management_client
# HEADERS=$(wildcard include/*.h)
# HEADERS2=$(wildcard include/**/*.h)
MGMT_SOURCES=$(wildcard management_client/*.c) tests/Utility.c


all: server sctp_client

server: $(SOURCES) $(SOURCES2)
	$(CC) $(CFLAGS) -g -pthread $(SOURCES) $(SOURCES2) -I$(INCLUDE) -L/usr/local/lib -lsctp -o $@
client: 
	$(CC) -g tests/socksClient.c tests/Utility.c -Iinclude -o socksClient
sctp_client: $(MGMT_SOURCES)
	$(CC) $(CFLAGS) -g $(MGMT_SOURCES) -I$(INCLUDE) -I$(MGMT_INCLUDE) -Itests/ -L/usr/local/lib -lsctp -o $@
clean:
	rm -rf application slaveProcess view sctp_client socksClient

# find . -type f -exec touch {} +
# valgrind --leak-check=yes --track-origins=yes --trace-children=yes ./sctp_client
# valgrind --leak-check=yes --track-origins=yes --trace-children=yes --tool=memcheck -v --log-file="valgrind-output.txt" ./server
# valgrind --leak-check=full --trace-children=yes --tool=memcheck --read-var-info=yes --show-reachable=yes -v --track-fds=yes --log-file="valgrind-output.txt" ./server
# DOH='--doh-port 80 --doh-path /dns-query --doh-host doh'