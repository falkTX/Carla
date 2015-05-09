#!/usr/bin/make -f
# Makefile for rtosc #
# ------------------ #
# Created by falkTX
#

CWD=../..
MODULENAME=rtosc
include ../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I.

# ----------------------------------------------------------------------------------------------------------------------------

OBJS = \
	$(OBJDIR)/dispatch.c.o \
	$(OBJDIR)/rtosc.c.o \
	$(OBJDIR)/midimapper.cpp.o \
	$(OBJDIR)/miditable.cpp.o \
	$(OBJDIR)/ports.cpp.o \
	$(OBJDIR)/subtree-serialize.cpp.o \
	$(OBJDIR)/thread-link.cpp.o \
	$(OBJDIR)/undo-history.cpp.o

# ----------------------------------------------------------------------------------------------------------------------------

all: $(MODULEDIR)/$(MODULENAME).a

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.a

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).a"
	@rm -f $@
	@$(AR) crs $@ $^

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.c.o: src/%.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.o: src/cpp/%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
