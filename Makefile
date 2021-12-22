TARGET := term_shapes
BUILD := ./build
OBJDIR := $(BUILD)/objects
BINDIR := $(BUILD)/bin
DEPDIR := $(OBJDIR)/.deps
BASE_SRC := src

BASE_FLAGS := -Wall -Werror -Wextra -pedantic-errors

# linker
LD := g++-11
# linker flags
LDFLAGS := -lm -lncurses
# linker flags: libraries to link (e.g. -lfoo)
LDLIBS :=
# flags required for dependency generation; passed to compilers
DEPFLAGS = -MT $@ -MD -MP -MF $(DEPDIR)/$*.Td

CTARGET := c_term_shapes
CC := gcc-11
CFLAGS := -std=c11 $(BASE_FLAGS)
CINCLUDE := -I$(BASE_SRC)/c/include
CSRC := $(wildcard $(BASE_SRC)/c/src/*.c)
COBJS := $(patsubst %,$(OBJDIR)/%.o,$(basename $(CSRC)))
CDEPS := $(patsubst %,$(DEPDIR)/%.d,$(basename $(CSRC)))

CXXTARGET := term_shapes
CXX := g++-11
CXXFLAGS := -std=c++17 $(BASE_FLAGS)
SYSINCLUDE := -isystem /usr/local/include/eigen3
CXXINCLUDE := -I$(BASE_SRC)/cpp/include
CXXSRC := $(wildcard $(BASE_SRC)/cpp/src/*.cc)
CXXOBJS := $(patsubst %,$(OBJDIR)/%.o,$(basename $(CXXSRC)))
CXXDEPS := $(patsubst %,$(DEPDIR)/%.d,$(basename $(CXXSRC)))

# compile C source files
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CINCLUDE) -c -o $@
# compile C++ source files
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CXXINCLUDE) $(SYSINCLUDE) -c -o $@
# link object files to binary
LINK.o = $(LD) $(LDFLAGS) -o $(BINDIR)/$@
# precompile step
PRECOMPILE =
# postcompile step
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

.PHONY: all c_impl clean # debug release

all: c cc

c: $(CTARGET)
	ln -sf $(BINDIR)/$(CTARGET) $(TARGET)

cc: $(CXXTARGET)
	ln -sf $(BINDIR)/$(CXXTARGET) $(TARGET)

# debug: CXXFLAGS += -DDEBUG -g
# debug: all

# release: CXXFLAGS += -O3
# release: all

clean:
	rm -rvf $(BUILD)
	rm -vf $(TARGET)
	rm -f log.txt

$(CTARGET): $(COBJS)
	-@mkdir -p $(BINDIR)
	$(LINK.o) $^

$(CXXTARGET): $(CXXOBJS)
	-@mkdir -p $(BINDIR)
	$(LINK.o) $^

$(OBJDIR)/%.o: %.c
$(OBJDIR)/%.o: %.c $(DEPDIR)/%.d
	$(shell mkdir -p $(dir $(COBJS)) >/dev/null)
	$(shell mkdir -p $(dir $(CDEPS)) >/dev/null)
	$(PRECOMPILE)
	$(COMPILE.c) $<
	$(POSTCOMPILE)

$(OBJDIR)/%.o: %.cc
$(OBJDIR)/%.o: %.cc $(DEPDIR)/%.d
	$(shell mkdir -p $(dir $(CXXOBJS)) >/dev/null)
	$(shell mkdir -p $(dir $(CXXDEPS)) >/dev/null)
	$(PRECOMPILE)
	$(COMPILE.cc) $<
	$(POSTCOMPILE)

.PRECIOUS: $(DEPDIR)/%.d
$(DEPDIR)/%.d: ;

-include $(CDEPS)
-include $(CCDEPS)