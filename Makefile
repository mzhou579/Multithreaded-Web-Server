# Chun Zhou
# This is the Makefile for CSE130 assignment 1
# Use to complie the C file httpserver

PROG	= httpserver
FlAGS	= -Wall -Wextra -Wpedantic -Wshadow 
SOURCES = $(PROG).c
OBJECTS = $(PROG).o
EXEBIN = $(PROG)

all: $(EXEBIN)

$(EXEBIN) : $(OBJECTS)
	gcc -o $(EXEBIN) $(OBJECTS) -lpthread

$(OBJECTS) : $(SOURCES)
	gcc -c $(FLAGS) $(SOURCES) 
	
clean : 
	rm $(EXEBIN) $(OBJECTS) 

