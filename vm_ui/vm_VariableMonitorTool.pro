QT += core gui widgets serialport
CONFIG += c++11 release
CONFIG -= debug debug_and_release debug_and_release_target
TEMPLATE = app
TARGET = VariableMonitorTool

ROOT = $$clean_path($$PWD/..)
DESTDIR = $$ROOT/build/qt/bin
OBJECTS_DIR = $$ROOT/build/qt/obj
MOC_DIR = $$ROOT/build/qt/moc
RCC_DIR = $$ROOT/build/qt/rcc
UI_DIR = $$ROOT/build/qt/ui

DEFINES += __USE_MINGW_ANSI_STDIO=1
QMAKE_CXXFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8

INCLUDEPATH += \
    $$PWD/include \
    $$PWD/port \
    $$ROOT/build/vm_elf_parser_make/include \
    $$ROOT/build/vm_com_make/include

SOURCES += \
    $$PWD/vm_main.cpp \
    $$PWD/src/vm_main_window.cpp \
    $$PWD/src/pages/vm_file_page.cpp \
    $$PWD/src/pages/vm_device_page.cpp \
    $$PWD/src/pages/vm_variable_load_page.cpp \
    $$PWD/src/pages/vm_monitor_page.cpp \
    $$PWD/src/pages/vm_calibration_page.cpp \
    $$PWD/src/pages/vm_log_page.cpp \
    $$PWD/port/vm_ui_port.cpp

HEADERS += \
    $$PWD/include/vm_main_window.h \
    $$PWD/include/pages/vm_page_types.h \
    $$PWD/include/pages/vm_file_page.h \
    $$PWD/include/pages/vm_device_page.h \
    $$PWD/include/pages/vm_variable_load_page.h \
    $$PWD/include/pages/vm_monitor_page.h \
    $$PWD/include/pages/vm_calibration_page.h \
    $$PWD/include/pages/vm_log_page.h \
    $$PWD/port/vm_ui_port.h \
    $$ROOT/build/vm_elf_parser_make/include/vm_monitor_variables.h \
    $$ROOT/build/vm_elf_parser_make/include/vm_status.h \
    $$ROOT/build/vm_com_make/include/vm_communication.h \
    $$ROOT/build/vm_com_make/include/vm_status.h

LIBS += \
    $$ROOT/build/vm_elf_parser_make/libvm_elf_parser.a \
    $$ROOT/build/vm_com_make/libvm_communication.a

RESOURCES += \
    $$PWD/resources/vm_app.qrc
