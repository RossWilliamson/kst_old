TOPOUT_REL=../../..
include($$PWD/$$TOPOUT_REL/kst.pri)
include($$PWD/../../../datasourceplugin.pri)

TARGET = $$kstlib(kst2data_netcdf4)
INCLUDEPATH += $$OUTPUT_DIR/src/datasources/netcdf4/tmp

SOURCES += \
    netcdf4.cpp

HEADERS += \
    netcdf4.h
