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
OUT = -o 

# configuration options
ifeq ($(MAKECMDGOALS),)
# config for 'all'; this should be used for deployment builds (LINK should strip as much out as possible)
COMPILE = g++ -c -O3 -Wall -pedantic-errors -Werror -Wextra -Wshadow -Wfatal-errors -Wno-unused-variable --std=gnu++0x
LINK = g++ -s
OBJDIR = $(OBJDIR_NAME)
MACROS = -DRAMSEY_POSIX
PROGRAM = $(PROGRAM_NAME)
else
# config for anything else (debug, test)
COMPILE = g++ -g -c -Wall -pedantic-errors -Werror -Wextra -Wshadow -Wfatal-errors -Wno-unused-variable --std=gnu++0x
LINK = g++
OBJDIR = $(OBJDIR_NAME_DEBUG)
MACROS = -DRAMSEY_DEBUG -DRAMSEY_POSIX
ifeq ($(MAKECMDGOALS),test)
PROGRAM = $(PROGRAM_NAME_TEST)
else
PROGRAM = $(PROGRAM_NAME_DEBUG)
endif
endif

# header file dependencies
RAMSEY_ERROR_H = ramsey-error.h
CODEGEN_H = codegen.h
GCCBUILD_H = gccbuild.h $(RAMSEY_ERROR_H)
LEXER_H = lexer.h $(RAMSEY_ERROR_H)
STABLE_H = stable.h $(LEXER_H)
AST_H = ast.h ast.tcc $(RAMSEY_ERROR_H) $(LEXER_H) $(STABLE_H)
PARSER_H = parser.h $(LEXER_H) $(AST_H)

# define all header files for testing
ALL_HEADER_FILES = lexer.h ramsey-error.h parser.h ast.h ast.tcc stable.h codegen.h

# object code files
OBJECTS = lexer.o parser.o ast.o ramsey-error.o stable.o semantics.o codegen.o
# add optional object code files depending on configuration
ifeq ($(MAKECMDGOALS),test)
OBJECTS := $(OBJECTS) test.o
else
OBJECTS := $(OBJECTS) ramsey.o gccbuild.o
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
$(OBJDIR)/parser.o: parser.cpp $(PARSER_H)
	$(COMPILE) $(MACROS) $(OUT)$(OBJDIR)/parser.o parser.cpp
$(OBJDIR)/ast.o: ast.cpp $(AST_H)
	$(COMPILE) $(MACROS) $(OUT)$(OBJDIR)/ast.o ast.cpp
$(OBJDIR)/ramsey-error.o: ramsey-error.cpp
	$(COMPILE) $(MACROS) $(OUT)$(OBJDIR)/ramsey-error.o ramsey-error.cpp
$(OBJDIR)/stable.o: stable.cpp $(STABLE_H)
	$(COMPILE) $(MACROS) $(OUT)$(OBJDIR)/stable.o stable.cpp
$(OBJDIR)/semantics.o: semantics.cpp $(AST_H)
	$(COMPILE) $(MACROS) $(OUT)$(OBJDIR)/semantics.o semantics.cpp
$(OBJDIR)/codegen.o: codegen.cpp $(CODEGEN_H) $(AST_H)
	$(COMPILE) $(MACROS) $(OUT)$(OBJDIR)/codegen.o codegen.cpp
$(OBJDIR)/test.o: test.cpp $(PARSER_H)
	$(COMPILE) $(MACROS) $(OUT)$(OBJDIR)/test.o test.cpp
$(OBJDIR)/ramsey.o: ramsey.cpp $(PARSER_H) $(GCCBUILD_H)
	$(COMPILE) $(MACROS) $(OUT)$(OBJDIR)/ramsey.o ramsey.cpp
$(OBJDIR)/gccbuild.o: gccbuild_posix.cpp $(GCCBUILD_H)
	$(COMPILE) $(MACROS) $(OUT)$(OBJDIR)/gccbuild.o gccbuild_posix.cpp

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

install:
	@if [ ! -f $(PROGRAM_NAME) ]; then echo Build project first && exit 1; fi;
	@cp --verbose $(PROGRAM_NAME) /usr/local/bin

uninstall:
	@rm --verbose /usr/local/bin/$(PROGRAM_NAME)
