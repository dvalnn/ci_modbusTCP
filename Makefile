# -*- Makefile -*-
# Makefile to build the project

# Parameters
CC = gcc

DEBUG_LEVEL = 0

# _DEBUG is used to include internal logging of errors and general information. Levels go from 1 to 3, highest to lowest priority respectively
CFLAGS = -Wall -g -D _DEBUG=$(DEBUG_LEVEL) #-Wno-unknown-pragmas -Wno-implicit-function-declaration -Wno-unused-variable
DEBUGFLAGS = -g -ggdb3

RUNARGS = 127.0.0.1 502

SRC = src
INCLUDE = include
LIB = lib
BIN = bin

APP = main.c
DEBUGEXTENS = dbg
BUILDEXTENS = exe

# Targets
.PHONY: all
all: $(BIN)/app.$(BUILDEXTENS)

$(BIN)/app.$(BUILDEXTENS): $(APP) $(SRC)/**/*.c $(LIB)/*c
	$(CC) $(CFLAGS) -o $@ $^ -I$(INCLUDE) -I$(LIB) -lrt

.PHONY: debug
debug: $(BIN)/app.$(DEBUGEXTENS)

$(BIN)/app.$(DEBUGEXTENS): $(APP) $(SRC)/**/*.c $(LIB)/*c
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o $@ $^ -I$(INCLUDE) -I$(LIB) -lrt


.PHONY: run
run:
	./$(BIN)/app.$(BUILDEXTENS) $(RUNARGS)

.PHONY: clean
clean:
	rm $(BIN)/*
	rm -f valgrind-out.txt


.PHONY: valgrind
valgrind:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./$(BIN)/app.$(DEBUGEXTENS) $(RUNARGS)