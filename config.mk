CXX = g++
DEPS_BIN = g++
CXXFLAGS = -O2 -std=c++11
LDFLAGS = -O2
LDLIBS = 
AR = ar
ARFLAGS = rc
MKDIR = mkdir
MKDIRFLAGS = -p

PREFIX = $(HOME)/.local/
BIN_DIR = bin/
INCLUDE_DIR = include/
LIB_DIR = lib/


PKG_NAME = alucelldb

SOURCES = src/db.cpp \
          src/alucell_legacy_database.cpp \
	  test/string.cpp \
	  test/write_dbfile.cpp

HEADERS = include/alucelldb/alucell_datatypes.hpp \
	  include/alucelldb/alucell_legacy_database.hpp \
	  include/alucelldb/alucell_legacy_variable.hpp \
	  include/alucelldb/string_utils.hpp \
	  include/alucelldb/alucell_database_index.hpp \
	  include/alucelldb/alucelldb.hpp

BIN = bin/db bin/test_string bin/test_write_dbfile

bin/db: build/src/db.o build/src/alucell_legacy_database.o
bin/test_string: build/test/string.o
bin/test_write_dbfile: build/test/write_dbfile.o build/src/alucell_legacy_database.o

LIB = lib/libalucelldb.a

lib/libalucelldb.a: build/src/alucell_legacy_database.o
