
CXX = g++
CXXFLAGS = -g -std=c++11
LDFLAGS = -g

BIN_DIR = ~/.local/bin/

BIN = bin/db
OBJECTS = build/db.o

.PHONY = all clean install

all: $(BIN)

bin/db: build/db.o
	$(CXX) $(LDFLAGS) -o $@ $<

build/db.o: src/db.cpp \
src/string_utils.hpp \
src/alucell_datatypes.hpp \
src/alucell_legacy_database.hpp \
src/alucell_legacy_variable.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS)
	rm -f $(BIN)

install: bin/db
	cp bin/db $(BIN_DIR)
