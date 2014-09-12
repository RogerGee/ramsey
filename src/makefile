################################################################################
# Makefile for CS355 Compiler Project ##########################################
################################################################################
.PHONY: install uninstall debug clean

# programs and options
PROGRAM_NAME = ramsey
PROGRAM_NAME_DEBUG = ramsey=debug
OBJDIR_NAME = obj
OBJDIR_NAME_DEBUG = dobj
LINK = g++
OUT = -o 

# configuration options
ifeq ($(MAKECMDGOALS),debug)
COMPILE = g++ -g -c -Wall -pedantic-errors -Werror -Wextra -Wshadow -Wfatal-errors -Wno-unused-variable --std=gnu++0x
OBJDIR = $(OBJDIR_NAME_DEBUG)
PROGRAM = $(PROGRAM_NAME_DEBUG)
else
COMPILE = g++ -c -Wall -pedantic-errors -Werror -Wextra -Wshadow -Wfatal-errors -Wno-unused-variable --std=gnu++0x
OBJDIR = $(OBJDIR_NAME)
PROGRAM = $(PROGRAM_NAME)
endif

# header file dependencies
LEXER_H = lexer.h

# object code files
OBJECTS = lexer.o
OBJECTS := $(addprefix $(OBJDIR)/,$(OBJECTS))

# target rules
all: $(OBJDIR) $(OBJECTS)
	$(LINK) $(OUT)$(PROGRAM) $(OBJECTS)
debug: $(OBJDIR) $(OBJECTS)
	$(LINK) $(OUT)$(PROGRAM) $(OBJECTS)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/lexer.o: lexer.cpp $(LEXER_H)
	$(COMPILE) $(OUT)$(OBJDIR)/lexer.o lexer.cpp

# special rules
clean:
	@if [ -d $(OBJDIR_NAME) ]; then rm -r --verbose $(OBJDIR_NAME); fi;
	@if [ -d $(OBJDIR_NAME_DEBUG) ]; then rm -r --verbose $(OBJDIR_NAME_DEBUG); fi;
	@if [ -f $(PROGRAM_NAME) ]; then rm --verbose $(PROGRAM_NAME); fi;
	@if [ -f $(PROGRAM_NAME_DEBUG) ]; then rm --verbose $(PROGRAM_NAME_DEBUG); fi;
