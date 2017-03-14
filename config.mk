CXX = g++
DEPS_BIN = g++
CXXFLAGS = -g -std=c++11
LDFLAGS = -g
AR = ar
ARFLAGS = rc

PREFIX = ~/.local/
BIN_DIR = bin/
INCLUDE_DIR = include/
LIB_DIR = lib/


SOURCES = src/db.cpp \
          src/alucell_legacy_database.cpp \
	  test/test_string.cpp

HEADERS = include/libalucelldb/alucell_datatypes.hpp \
	  include/libalucelldb/alucell_legacy_database.hpp \
	  include/libalucelldb/alucell_legacy_variable.hpp \
	  include/libalucelldb/string_utils.hpp \
	  include/libalucelldb/alucelldb.hpp

BIN = bin/db bin/test_string

bin/db: build/src/db.o build/src/alucell_legacy_database.o
bin/test_string: build/test/test_string.o

LIB = 
