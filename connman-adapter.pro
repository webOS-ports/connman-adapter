TEMPLATE = app

CONFIG += qt

TARGET_TYPE =

CONFIG += link_pkgconfig
PKGCONFIG = glib-2.0 gthread-2.0 luna-service2 cjson

QT = core dbus

SOURCES = \
    src/main.cpp \
    src/servicemgr.cpp \
    src/networkmanager.cpp \
    src/connmanmanager.cpp \
    src/commondbustypes.cpp \
    src/utilities.cpp

HEADERS = \
    src/servicemgr.h \
    src/networkmanager.h \
    src/connmanmanager.h \
    src/commondbustypes.h \
    src/utilities.h

TARGET = connman-adapter

OBJECTS_DIR = .obj
MOC_DIR = .moc
