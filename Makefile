
.SUFFIX:

include config.mk

OBJECTS = $(patsubst %.cpp,build/%.o,$(SOURCES))
DEPS = $(patsubst %.cpp,build/%.deps,$(SOURCES))

.PHONY = all deps clean install

all: $(BIN) $(LIB)

-include $(DEPS)

$(HEADERS): include/libalucelldb/%: src/%
	@echo "[INST]" $(<:src/%=%)
	@install -m 0644 -D $< $@


$(OBJECTS): build/%.o: %.cpp
	@echo "[CXX] " $@
	@mkdir --parents $(dir $@)
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

$(DEPS): build/%.deps: %.cpp
	@echo "[DEPS]" $@
	@mkdir --parents $(dir $@)
	@$(DEPS_BIN) -std=c++11 -MM -MT build/$*.o $< > $@
	@$(DEPS_BIN) -std=c++11 -MM -MT build/$*.deps $< >> $@

$(BIN): bin/%:
	@echo "[LD]  " $@
	@$(CXX) $(LDFLAGS) -o $@ $^

$(LIB): lib/%:
	@echo "[AR]  " $@
	@$(AR) $(ARFLAGS) $@ $^

deps: $(DEPS)

clean:
	@rm -f $(OBJECTS)
	@rm -f $(DEPS)
	@rm -f $(BIN)
	@rm -f $(LIB)
	@rm -rf include/*
	@rm -rf build/*

install: $(BIN) $(HEADERS) $(LIB)
	cp $(BIN) $(PREFIX)/$(BIN_DIR)
	cp -r include/* $(PREFIX)/$(INCLUDE_DIR)
	cp $(LIB) $(PREFIX)/$(LIB_DIR)/
