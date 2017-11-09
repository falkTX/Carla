#!/usr/bin/make -f
# Makefile for audio_decoder #
# -------------------------- #
# Created by falkTX
#

CWD=../..
MODULENAME=audio_decoder
include ../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

# BUILD_C_FLAGS +=

# ----------------------------------------------------------------------------------------------------------------------------

OBJS = \
	$(OBJDIR)/ad_ffmpeg.c.o \
	$(OBJDIR)/ad_plugin.c.o \
	$(OBJDIR)/ad_soundfile.c.o

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

$(OBJDIR)/ad_ffmpeg.c.o: ad_ffmpeg.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) $(FFMPEG_FLAGS) -c -o $@

$(OBJDIR)/ad_plugin.c.o: ad_plugin.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(OBJDIR)/ad_soundfile.c.o: ad_soundfile.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) $(SNDFILE_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
