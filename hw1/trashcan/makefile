# 046267 Computer Architecture - HW #1
# Makefile for test environment

all: bp_main

# Environment for C 
CC = gcc
CFLAGS = -std=c99 -Wall

# Environment for C++ 
CXX = g++
CXXFLAGS = -std=c++11 -Wall

# Automatically detect whether the bp is C or C++
# Must have either bp.c or bp.cpp - NOT both
SRC_BP = $(wildcard bp.c bp.cpp)
SRC_GIVEN = bp_main.c
EXTRA_DEPS = bp_api.h

OBJ_GIVEN = $(patsubst %.c,%.o,$(SRC_GIVEN))
OBJ_BP = bp.o
OBJ = $(OBJ_GIVEN) $(OBJ_BP)

# Include dependencies
DEPS = $(OBJ:.o=.d)

# Conditional compilation based on source type
ifeq ($(SRC_BP),bp.c)
bp_main: $(OBJ)
	$(CC) -o $@ $(OBJ) -lm -g

bp.o: bp.c
	$(CC) -c $(CFLAGS) -MMD -MP -g -o $@ $<

else
bp_main: $(OBJ)
	$(CXX) -g -o $@ $(OBJ)

bp.o: bp.cpp
	$(CXX) -c $(CXXFLAGS) -MMD -MP -g -o $@ $<

endif

-include $(DEPS)

$(OBJ_GIVEN): %.o: %.c
	$(CC) -c -g $(CFLAGS) -MMD -MP -o $@ $<

.PHONY: clean
clean:
	rm -f bp_main $(OBJ) $(DEPS)
