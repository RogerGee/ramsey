################################################################################
# Makefile for CS355 Compiler Project ##########################################
################################################################################
.PHONY: install uninstall debug test clean

# programs and options
PROGRAM_NAME = ramsey
PROGRAM_NAME_DEBUG = ramsey-debug
PROGRAM_NAME_TEST = ramsey-test
OBJDIR_NAME = obj
OBJDIR_NAME_DEBUG = dobj
LINK = g++
OUT = -o 

# configuration options
ifeq ($(MAKECMDGOALS),)
# config for 'all'
COMPILE = g++ -c -Wall -pedantic-errors -Werror -Wextra -Wshadow -Wfatal-errors -Wno-unused-variable --std=gnu++0x
OBJDIR = $(OBJDIR_NAME)
PROGRAM = $(PROGRAM_NAME)
else
# config for anything else (debug, test)
COMPILE = g++ -g -c -Wall -pedantic-errors -Werror -Wextra -Wshadow -Wfatal-errors -Wno-unused-variable --std=gnu++0x
OBJDIR = $(OBJDIR_NAME_DEBUG)
MACROS = -DRAMSEY_DEBUG
ifeq ($(MAKECMDGOALS),test)
PROGRAM = $(PROGRAM_NAME_TEST)
else
PROGRAM = $(PROGRAM_NAME_DEBUG)
endif
endif

# header file dependencies
ALL_HEADER_FILES = lexer.h
LEXER_H = lexer.h

# object code files
OBJECTS = lexer.o
# add optional object code files depending on configuration
ifeq ($(MAKECMDGOALS),test)
OBJECTS := $(OBJECTS) test.o
endif
OBJECTS := $(addprefix $(OBJDIR)/,$(OBJECTS))

# main target rules
all: $(OBJDIR) $(PROGRAM)
debug: $(OBJDIR) $(PROGRAM)
test: $(OBJDIR) $(PROGRAM)

# build program
$(PROGRAM): $(OBJECTS)
	$(LINK) $(OUT)$(PROGRAM) $(OBJECTS)

# build program modules
$(OBJDIR)/lexer.o: lexer.cpp $(LEXER_H)
	$(COMPILE) $(MACROS) $(OUT)$(OBJDIR)/lexer.o lexer.cpp
$(OBJDIR)/test.o: test.cpp $(ALL_HEADER_FILES)
	$(COMPILE) $(MACROS) $(OUT)$(OBJDIR)/test.o test.cpp

# other targets
$(OBJDIR):
	@mkdir $(OBJDIR)

# special rules
clean:
	@if [ -d $(OBJDIR_NAME) ]; then rm -r --verbose $(OBJDIR_NAME); fi;
	@if [ -d $(OBJDIR_NAME_DEBUG) ]; then rm -r --verbose $(OBJDIR_NAME_DEBUG); fi;
	@if [ -f $(PROGRAM_NAME) ]; then rm --verbose $(PROGRAM_NAME); fi;
	@if [ -f $(PROGRAM_NAME_DEBUG) ]; then rm --verbose $(PROGRAM_NAME_DEBUG); fi;
	@if [ -f $(PROGRAM_NAME_TEST) ]; then rm --verbose $(PROGRAM_NAME_TEST); fi;
