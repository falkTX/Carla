#!/usr/bin/make -f
# Makefile for carla-frontend #
# --------------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin
RESDIR    := $(CWD)/../resources

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/frontend/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/frontend/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ---------------------------------------------------------------------------------------------------------------------
# UI code

UIs = \
	ui_carla_about.py \
	ui_carla_add_jack.py \
	ui_carla_database.py \
	ui_carla_edit.py \
	ui_carla_host.py \
	ui_carla_parameter.py \
	ui_carla_plugin_calf.py \
	ui_carla_plugin_classic.py \
	ui_carla_plugin_compact.py \
	ui_carla_plugin_default.py \
	ui_carla_plugin_presets.py \
	ui_carla_refresh.py \
	ui_carla_settings.py \
	ui_carla_settings_driver.py \
	ui_inputdialog_value.py \
	ui_midipattern.py

# ---------------------------------------------------------------------------------------------------------------------
# Resources

RES = \
	carla_config.py \
	resources_rc.py \
	$(BINDIR)/resources/widgets \
	$(BINDIR)/resources/carla_app.py \
	$(BINDIR)/resources/carla_backend.py \
	$(BINDIR)/resources/carla_backend_qt.py \
	$(BINDIR)/resources/carla_config.py \
	$(BINDIR)/resources/carla_control.py \
	$(BINDIR)/resources/carla_database.py \
	$(BINDIR)/resources/carla_host.py \
	$(BINDIR)/resources/carla_settings.py \
	$(BINDIR)/resources/carla_skin.py \
	$(BINDIR)/resources/carla_shared.py \
	$(BINDIR)/resources/carla_utils.py \
	$(BINDIR)/resources/carla_widgets.py \
	$(BINDIR)/resources/externalui.py \
	$(BINDIR)/resources/patchcanvas.py \
	$(BINDIR)/resources/patchcanvas_theme.py \
	$(BINDIR)/resources/resources_rc.py \
	$(BINDIR)/resources/ui_carla_about.py \
	$(BINDIR)/resources/ui_carla_add_jack.py \
	$(BINDIR)/resources/ui_carla_database.py \
	$(BINDIR)/resources/ui_carla_edit.py \
	$(BINDIR)/resources/ui_carla_host.py \
	$(BINDIR)/resources/ui_carla_parameter.py \
	$(BINDIR)/resources/ui_carla_plugin_calf.py \
	$(BINDIR)/resources/ui_carla_plugin_classic.py \
	$(BINDIR)/resources/ui_carla_plugin_compact.py \
	$(BINDIR)/resources/ui_carla_plugin_default.py \
	$(BINDIR)/resources/ui_carla_plugin_presets.py \
	$(BINDIR)/resources/ui_carla_refresh.py \
	$(BINDIR)/resources/ui_carla_settings.py \
	$(BINDIR)/resources/ui_carla_settings_driver.py \
	$(BINDIR)/resources/ui_inputdialog_value.py \
	$(BINDIR)/resources/ui_midipattern.py

# ----------------------------------------------------------------------------------------------------------------------

all: $(UIs) $(RES)

# ----------------------------------------------------------------------------------------------------------------------

ui_%.py: $(RESDIR)/ui/%.ui
	$(PYUIC) $< -o $@

carla_config.py:
	@echo "#!/usr/bin/env python3" > $@
	@echo "# -*- coding: utf-8 -*-" >> $@
ifeq ($(DEFAULT_QT),4)
	@echo "config_UseQt5 = False" >> $@
else
	@echo "config_UseQt5 = True" >> $@
endif

resources_rc.py: $(RESDIR)/resources.qrc $(RESDIR)/*/*.png $(RESDIR)/*/*.svg
	$(PYRCC) $< -o $@

$(BINDIR)/resources/%: %
	$(LINK) $(CURDIR)/$* $(BINDIR)/resources/

# ----------------------------------------------------------------------------------------------------------------------

clean:
	rm -rf $(UIs) $(RES) __pycache__ *.pyc

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------
