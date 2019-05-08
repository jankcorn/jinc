.PHONY: default test
default: test

ifneq ($(filter help,$(MAKECMDGOALS)),)

.PHONY: help
help:
	@echo 'Usage: make [options] [target] ...'
	@echo 'Options:'
	@echo '  -j [N], --jobs[=N]          Allow N jobs at once; infinite jobs with no arg'
	@echo '  -l [N], --load-average[=N], --max-load[=N]'
	@echo '                              Don'\''t start multiple jobs unless load is below N'
	@echo '  -h, --help                  Print help on options'
	@echo '  ...                         (really, try -h)'
	@echo 'Targets:'
	@echo '  help                        Print this message and exit'
	@echo '  all                         Make everthing'
	@echo '  lib                         Create library'
	@echo '  bin                         Create binaries'
	@echo '  test                        Run all binaries, check success'
	@echo '  clean                       Clean up tree, remove everything generated and any backup files'
	@echo '  tgz                         Calls clean, then creates an archive in the parent directory'
	@echo
	@echo 'Do not mix clean or tgz with other targets'

else

basename := $(notdir $(CURDIR))

ifneq ($(filter clean tgz,$(MAKECMDGOALS)),)

ifneq ($(filter-out clean tgz,$(MAKECMDGOALS)),)
$(error "Can't mix targets `clean' or `tgz' with other targets")
endif

.PHONY: clean
clean:
	@rm -rf build lib bin test
	@find . -name '*~' -delete -type f
	@dot_clean .
.PHONY: tgz
tgz: clean
	( cd .. && export COPYFILE_DISABLE=true && tar -c -z --exclude '\.svn' --exclude '\.DS_Store' --exclude '\._*' -f $(basename)_$(shell date -u +'%F_%H%M%S').tgz $(basename) )

else # clean/tgz

libname := $(basename:%=lib%.so)

lib_sources := $(shell find src/lib -type f -name '*.cpp')
bin_sources := $(shell find src/bin -type f -name '*.cpp')
sources := $(lib_sources) $(bin_sources)

CXXCPP ?= g++
CXX ?= g++
CXXLD ?= g++

# Separate compiler options
include makefile.options

build/%.d: %.cpp Makefile makefile.*
	@mkdir -p $(@D)
	$(CXXCPP) $(CPPFLAGS) -MM -o- $< | sed 's#^\(.*\)\.o: #$(@D)/\1.d: #' >$@

-include $(sources:%.cpp=build/%.d)

# removed headers might still be listed in .d-files
# this rule allows the .d-files to be regenerated automatically
%.hpp:
	@true

build/%.o: %.cpp build/%.d
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

lib/%: $(lib_sources:%.cpp=build/%.o)
	@mkdir -p $(@D)
	$(CXXLD) $(LDFLAGS) $(LIBS) -dynamiclib -o $@ $^
	# not for MAC OS X @strip $@

bin/%: build/src/bin/%.o lib/$(libname)
	@mkdir -p $(@D)
	$(CXX) $(LIBS) -Llib -l$(basename) -o $@ $<
	#@strip $@

test/%.log: bin/%
	@mkdir -p $(@D)
	LD_LIBRARY_PATH=lib:$$LD_LIBRARY_PATH $< >$@ 2>&1 || ( mv $@ $@.failed && false )

.PHONY: lib
lib: lib/$(libname)

.PHONY: bin
bin: $(bin_sources:src/bin/%.cpp=bin/%)

.PHONY: test
test: $(bin_sources:src/bin/%.cpp=test/%.log)

.PHONY: all
all: bin test

# prevent removing intermediate files
.SECONDARY: $(sources:%.cpp=build/%.o)

endif # clean/tgz

endif # help
