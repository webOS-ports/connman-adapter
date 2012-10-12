TEMPLATE = app

CONFIG += qt

TARGET_TYPE =

CONFIG += link_pkgconfig
PKGCONFIG = glib-2.0 gthread-2.0 luna-service2 connman-qt4

QT = core dbus

SOURCES = \
    src/main.cpp \
    src/servicemgr.cpp \
    src/networkingmodel.cpp

HEADERS = \
    src/servicemgr.h \
    src/networkingmodel.h

TARGET = connman-adapter
