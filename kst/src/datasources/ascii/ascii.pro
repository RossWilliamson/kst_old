TOPOUT_REL=../../..
include($$PWD/$$TOPOUT_REL/kst.pri)
include($$PWD/../../../datasourceplugin.pri)

TARGET = $$kstlib(kst2data_ascii)
INCLUDEPATH += $$OUTPUT_DIR/src/datasources/ascii/tmp

SOURCES += \
  asciisource.cpp \
  asciisourceconfig.cpp \
  asciiplugin.cpp

win32:SOURCES += kst_atof.cpp

HEADERS += \
  asciisource.h \
  asciisourceconfig.h \
  asciiplugin.h \
  namedparameter.h \
  ../../libkst/kst_inf.h

win32:HEADERS += kst_atof.h
FORMS += asciiconfig.ui