#!/usr/bin/make -f
# Makefile for carla-frontend #
# --------------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BINDIR := $(CWD)/../bin
RESDIR := $(CWD)/../resources

# ---------------------------------------------------------------------------------------------------------------------
# Translations

TSs = $(patsubst %,translations/carla_%.ts,$(I18N_LANGUAGES))
QMs = $(patsubst %,translations/carla_%.qm,$(I18N_LANGUAGES))

# ---------------------------------------------------------------------------------------------------------------------
# Resources

RES = \
	resources_rc.py \
	$(BINDIR)/resources/modgui \
	$(BINDIR)/resources/patchcanvas \
	$(BINDIR)/resources/widgets \
	$(BINDIR)/resources/bigmeter-ui \
	$(BINDIR)/resources/carla-plugin \
	$(BINDIR)/resources/carla-plugin-patchbay \
	$(BINDIR)/resources/carla_app.py \
	$(BINDIR)/resources/carla_backend.py \
	$(BINDIR)/resources/carla_backend_qt.py \
	$(BINDIR)/resources/carla_database.py \
	$(BINDIR)/resources/carla_host.py \
	$(BINDIR)/resources/carla_host_control.py \
	$(BINDIR)/resources/carla_modgui.py \
	$(BINDIR)/resources/carla_settings.py \
	$(BINDIR)/resources/carla_skin.py \
	$(BINDIR)/resources/carla_shared.py \
	$(BINDIR)/resources/carla_utils.py \
	$(BINDIR)/resources/carla_widgets.py \
	$(BINDIR)/resources/externalui.py \
	$(BINDIR)/resources/midipattern-ui \
	$(BINDIR)/resources/notes-ui \
	$(BINDIR)/resources/xycontroller-ui \
	$(BINDIR)/resources/resources_rc.py \
	$(BINDIR)/resources/ui_carla_about.py \
	$(BINDIR)/resources/ui_carla_about_juce.py \
	$(BINDIR)/resources/ui_carla_add_jack.py \
	$(BINDIR)/resources/ui_carla_database.py \
	$(BINDIR)/resources/ui_carla_edit.py \
	$(BINDIR)/resources/ui_carla_host.py \
	$(BINDIR)/resources/ui_carla_osc_connect.py \
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
	$(BINDIR)/resources/ui_midipattern.py \
	$(BINDIR)/resources/ui_xycontroller.py

ifneq ($(SKIP_ZYN_SYNTH),true)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
RES += \
	$(BINDIR)/resources/zynaddsubfx \
	$(BINDIR)/resources/zynaddsubfx-ui
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# UI code

UIs = \
	ui_carla_about.py \
	ui_carla_about_juce.py \
	ui_carla_add_jack.py \
	ui_carla_database.py \
	ui_carla_edit.py \
	ui_carla_host.py \
	ui_carla_osc_connect.py \
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
	ui_midipattern.py \
	ui_xycontroller.py

# ---------------------------------------------------------------------------------------------------------------------

all: $(QMs) $(RES) $(UIs)

# ---------------------------------------------------------------------------------------------------------------------

ui_%.py: $(RESDIR)/ui/%.ui
	$(PYUIC) $< -o $@

resources_rc.py: $(RESDIR)/resources.qrc $(RESDIR)/*/*.png $(RESDIR)/*/*.svg $(RESDIR)/*/*.svgz
	$(PYRCC) $< -o $@

$(BINDIR)/resources/%: %
	-@mkdir -p $(BINDIR)/resources
	$(LINK) $(CURDIR)/$* $(BINDIR)/resources/

$(BINDIR)/resources/zynaddsubfx: ../native-plugins/resources/zynaddsubfx
	-@mkdir -p $(BINDIR)/resources
	$(LINK) $(CURDIR)/$< $(BINDIR)/resources/

$(BINDIR)/resources/zynaddsubfx-ui: ../native-plugins/resources/zynaddsubfx-ui
	-@mkdir -p $(BINDIR)/resources
	$(LINK) $(CURDIR)/$< $(BINDIR)/resources/

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -rf $(UIs) $(RES) $(QMs) __pycache__ *.pyc

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

i18n_update: $(TSs)
i18n_release: $(QMs)

translations/%.ts:
	@install -d translations
	pylupdate5 *.py $(RESDIR)/ui/*.ui -ts $@

%.qm: %.ts
	lrelease $< -qm $@

# ---------------------------------------------------------------------------------------------------------------------
