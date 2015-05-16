set(UI_fl_files
    ADnoteUI.fl
    BankUI.fl
    ConfigUI.fl
    EffUI.fl
    EnvelopeUI.fl
    FilterUI.fl
    LFOUI.fl
    MasterUI.fl
    MicrotonalUI.fl
    OscilGenUI.fl
    PADnoteUI.fl
    PartUI.fl
    PresetsUI.fl
    ResonanceUI.fl
    SUBnoteUI.fl
    VirKeyboard.fl
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set_source_files_properties(UI/MasterUI.h PROPERTIES GENERATED 1)
fltk_wrap_ui(zynaddsubfx_gui ${UI_fl_files})

if(LibloEnable)
    set(zynaddsubfx_gui_FLTK_UI_SRCS ${zynaddsubfx_gui_FLTK_UI_SRCS} NSM.C NSM/Client.C)
endif()
if(NtkGui)
    add_definitions(-DPIXMAP_PATH="${CMAKE_INSTALL_PREFIX}/share/zynaddsubfx/pixmaps/")
    add_definitions(-DSOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif()

if(FltkGui)
    add_executable(zynaddsubfx_ext_gui guimain.cpp)
    target_link_libraries(zynaddsubfx_ext_gui zynaddsubfx_gui ${FLTK_LIBRARIES}
        ${FLTK_LIBRARIES} ${OPENGL_LIBRARIES} ${X11_LIBRARIES} ${LIBLO_LIBRARIES} rtosc rtosc-cpp)
endif()

add_library(zynaddsubfx_gui STATIC
	${UI_objs}
	${zynaddsubfx_gui_FLTK_UI_SRCS}
        NioUI.cpp
        WidgetPDial.cpp
        PartNameButton.cpp
        Fl_Osc_Pane.cpp
        Fl_Osc_Widget.cpp
        Fl_Osc_Dial.cpp
        Fl_Osc_DialF.cpp
        Fl_Osc_Slider.cpp
        Fl_Osc_VSlider.cpp
        Fl_Osc_Button.cpp
        Fl_Osc_Check.cpp
        Fl_Osc_Choice.cpp
        Fl_Osc_Roller.cpp
        Fl_Osc_Output.cpp
        Fl_Osc_Counter.cpp
        Fl_Osc_Input.cpp
        Fl_Osc_Value.cpp
        Fl_Osc_ListView.cpp
        Fl_Resonance_Graph.cpp
        Fl_EQGraph.cpp
        FormantFilterGraph.cpp
        EnvelopeFreeEdit.cpp
        BankView.cpp
	)
add_library(zynaddsubfx_gui_bridge STATIC
    Connection.cpp)
add_dependencies(zynaddsubfx_gui_bridge zynaddsubfx_gui)


if(NtkGui)
  target_link_libraries(zynaddsubfx_gui ${NTK_LIBRARIES} ${NTK_IMAGES_LIBRARIES})
endif(NtkGui)

if(FltkGui)
  target_link_libraries(zynaddsubfx_gui ${FLTK_LIBRARIES})
endif(FltkGui)
