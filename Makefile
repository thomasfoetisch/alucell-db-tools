
CXX = g++
CXXFLAGS = -g -std=c++11
LDFLAGS = -g

BIN_DIR = ~/.local/bin/

BIN = bin/db
OBJECTS = build/src/db.o

.PHONY = all clean install

HEADERS = include/libalucelldb/alucell_datatypes.hpp \
	  include/libalucelldb/alucell_legacy_database.hpp \
	  include/libalucelldb/alucell_legacy_variable.hpp \
	  include/libalucelldb/string_utils.hpp

$(HEADERS): include/libalucelldb/%: src/%
	@echo [INSTALL] $(<:src/%=%)
	@install -m 0644 -D $< $@

all: $(BIN) $(HEADERS)

$(OBJECTS): build/%.o: %.cpp
	@echo [CXX] $<
	@mkdir --parents $(dir $@)
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN): bin/%: build/src/%.o
	@echo [LD] $@
	@$(CXX) $(LDFLAGS) -o $@ $<

$(BIN_TEST): bin/%: build/test/%.o
	@echo [LD] $@
	@$(CXX) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(OBJECTS)
	rm -f $(BIN)
	rm -rf include/*

install: bin/db
	cp bin/db $(BIN_DIR)
