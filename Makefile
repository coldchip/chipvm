CC      := gcc
LD      := ld
BIN     := bin
SRCS    := $(wildcard *.c)
EXE     := $(BIN)/vm
CFLAGS  := -Wall -std=c99 -Ofast -s
LIBS    := 
ifeq ($(OS),Windows_NT)
	LIBS := $(LIBS) -lws2_32
endif

.PHONY: clean install

all: 
	./build.sh
run:
	$(EXE)
clean:
	rm -rf bin/*
