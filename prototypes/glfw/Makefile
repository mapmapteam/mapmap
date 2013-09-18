#!/bin/bash

DEPS=glfw3 glu gl
CC=gcc
CC_FLAGS=-O0 -g `pkg-config --cflags --libs $(DEPS)` -lSOIL
EXEC=run
SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)

all: run

$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) $(CC_FLAGS) -o $(EXEC)

%.o: %.c
	$(CC) -c $(CC_FLAGS) $< -o $@

clean:
	rm -f $(EXEC) $(OBJECTS)

