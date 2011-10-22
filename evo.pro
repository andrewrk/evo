QT       -= gui
QT       += core

TARGET = evo
CONFIG   += console
CONFIG   -= app_bundle

SOURCES += src/main.cpp \
    src/Instruction.cpp \
    src/Interpreter.cpp \
    src/CmdLineOptions.cpp \
    src/Tape.cpp

HEADERS += \
    src/Instruction.h \
    src/Interpreter.h \
    src/CmdLineOptions.h \
    src/Tape.h \
    src/instructions.h






