TEMPLATE = app

CONFIG += qt

TARGET_TYPE =

CONFIG += link_pkgconfig
PKGCONFIG = glib-2.0 gthread-2.0 luna-service2 cjson connman-qt4

QT = core dbus

SOURCES = \
    src/main.cpp \
    src/servicemgr.cpp \
    src/wifiservice.cpp \
    src/connmanagent.cpp \
    src/utilities.cpp

HEADERS = \
    src/servicemgr.h \
    src/wifiservice.h \
    src/connmanagent.h \
    src/utilities.h

TARGET = connman-adapter

OBJECTS_DIR = .obj
MOC_DIR = .moc
