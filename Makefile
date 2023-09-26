# -*- Makefile -*-
# Makefile to build the project

# Parameters
CC = gcc

DEBUG_LEVEL = 1

# _DEBUG is used to include internal logging of errors and general information. Levels go from 1 to 3, highest to lowest priority respectively
CFLAGS = -Wall -g -D _DEBUG=$(DEBUG_LEVEL) #-Wno-unknown-pragmas -Wno-implicit-function-declaration -Wno-unused-variable

SRC = src
INCLUDE = include
LIB = lib
BIN = bin

APP = main.c
BUILDEXTENS = exe

# Targets
.PHONY: all
all: $(BIN)/app.$(BUILDEXTENS)

$(BIN)/app.$(BUILDEXTENS): $(APP) $(SRC)/*.c
	$(CC) $(CFLAGS) -o $@ $^ -I$(INCLUDE) -lrt

.PHONY: run
run:
	./$(BIN)/app.$(BUILDEXTENS)

.PHONY: clean
clean:
	rm -f $(BIN)/*
	rm $(OUTFILE)