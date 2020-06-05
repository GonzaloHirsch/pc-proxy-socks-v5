GCC=gcc
GCCFLAGS=-Wall -g
# LDFLAGS=-lrt -pthread

SOURCES=$(wildcard src/*.c)
SOURCES_ASM=$(wildcard asm/*.asm)
OBJECTS=$(SOURCES:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
LOADERSRC=loader.asm


clean:
        rm -rf application slaveProcess view
