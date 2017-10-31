
.SUFFIX:

include config.mk

OBJECTS = $(patsubst %.cpp,build/%.o,$(SOURCES))
DEPS = $(patsubst %.cpp,build/%.deps,$(SOURCES))

.PHONY = all deps clean install install-all install-lib install-bin install-header
.DEFAULT_GOAL = all

all: $(BIN) $(LIB) $(HEADERS)

-include $(DEPS)

$(HEADERS): include/$(PKG_NAME)/%: src/%
	@echo "[INST]" $(<:src/%=%)
	@$(MKDIR) $(MKDIRFLAGS) $(dir $@)
	@cp $< $(dir $@)


$(OBJECTS): build/%.o: %.cpp
	@echo "[CXX] " $@
	@$(MKDIR) $(MKDIRFLAGS) $(dir $@)
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

$(DEPS): build/%.deps: %.cpp
	@echo "[DEPS]" $@
	@$(MKDIR) $(MKDIRFLAGS) $(dir $@)
	@$(DEPS_BIN) -std=c++11 -MM -MT build/$*.o $< > $@
	@$(DEPS_BIN) -std=c++11 -MM -MT build/$*.deps $< >> $@

$(BIN): bin/%:
	@echo "[LD]  " $@
	@$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

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

install: install-dev
install-dev: install-header install-lib
install-all: install-bin install-lib install-header

install-bin: $(BIN)
	@echo "[CP]  " $@
ifneq ($(BIN),)
	@$(MKDIR) $(MKDIRFLAGS) $(PREFIX)/$(BIN_DIR)
	@cp $(BIN) $(PREFIX)/$(BIN_DIR)
endif

install-header: $(HEADERS)
	@echo "[CP]  " $@
ifneq ($(HEADERS),)
	@$(MKDIR) $(MKDIRFLAGS) $(PREFIX)/$(INCLUDE_DIR)
	@cp -r include/* $(PREFIX)/$(INCLUDE_DIR)
endif

install-lib: $(LIB)
	@echo "[CP]  " $@
ifneq ($(LIB),)
	@$(MKDIR) $(MKDIRFLAGS) $(PREFIX)/$(LIB_DIR)
	@cp $(LIB) $(PREFIX)/$(LIB_DIR)/
endif

print-%:
	@echo $*=$($*)
