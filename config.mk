CXX = g++
DEPS_BIN = g++
CXXFLAGS = -g -std=c++11
LDFLAGS = -g
LDLIBS = 
AR = ar
ARFLAGS = rc
MKDIR = mkdir
MKDIRFLAGS = -p

PREFIX = ~/.local/
BIN_DIR = bin/
INCLUDE_DIR = include/
LIB_DIR = lib/


SOURCES = src/db.cpp \
          src/alucell_legacy_database.cpp \
	  test/test_string.cpp \
	  test/write_dbfile.cpp

HEADERS = include/libalucelldb/alucell_datatypes.hpp \
	  include/libalucelldb/alucell_legacy_database.hpp \
	  include/libalucelldb/alucell_legacy_variable.hpp \
	  include/libalucelldb/string_utils.hpp \
	  include/libalucelldb/alucelldb.hpp

BIN = bin/db bin/test_string bin/test_write_dbfile

bin/db: build/src/db.o build/src/alucell_legacy_database.o
bin/test_string: build/test/test_string.o
bin/test_write_dbfile: build/test/write_dbfile.o build/src/alucell_legacy_database.o

LIB = lib/libalucelldb.a

lib/libalucelldb.a: build/src/alucell_legacy_database.o
