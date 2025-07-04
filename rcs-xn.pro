TARGET = rcs-xn
TEMPLATE = lib
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
	src/rcs-xn.cpp \
	src/rcs-xn-gui.cpp \
	src/settings.cpp \
	src/signals.cpp \
	src/form-signal-edit.cpp \
	src/lib-api.cpp
HEADERS += \
	src/rcs-xn.h \
	src/errors.h \
	src/events.h \
	src/settings.h \
	src/util.h \
	src/signals.h \
	src/form-signal-edit.h \
	src/lib-api.h \
	src/lib-api-common-def.h \
	src/q-tree-num-widget-item.h

FORMS += \
	form/main-window.ui \
	form/signal-edit.ui

SOURCES += \
	lib/xn-lib-cpp-qt/xn-pending.cpp \
	lib/xn-lib-cpp-qt/xn.cpp \
	lib/xn-lib-cpp-qt/xn-api.cpp \
	lib/xn-lib-cpp-qt/xn-receive.cpp \
	lib/xn-lib-cpp-qt/xn-send.cpp \
	lib/xn-lib-cpp-qt/xn-win-com-discover.cpp
HEADERS += \
	lib/xn-lib-cpp-qt/q-str-exception.h \
	lib/xn-lib-cpp-qt/xn-loco-addr.h \
	lib/xn-lib-cpp-qt/xn-commands.h \
	lib/xn-lib-cpp-qt/xn.h \
	lib/q-str-exception.h \
	lib/xn-lib-cpp-qt/xn-win-com-discover.h

CONFIG += c++14 dll
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic

win32 {
	QMAKE_LFLAGS += -Wl,--kill-at
	QMAKE_CXXFLAGS += -enable-stdcall-fixup
	LIBS += -lsetupapi
}
win64 {
	QMAKE_LFLAGS += -Wl,--kill-at
	QMAKE_CXXFLAGS += -enable-stdcall-fixup
	LIBS += -lsetupapi
}

QT += core gui serialport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

VERSION_MAJOR = 1
VERSION_MINOR = 6

DEFINES += "VERSION_MAJOR=$$VERSION_MAJOR" \
	"VERSION_MINOR=$$VERSION_MINOR" \

# Uncomment for official release without '-dev' in caption
# DEFINES += "RCS_XN_RELEASE"

#Target version
VERSION = "$${VERSION_MAJOR}.$${VERSION_MINOR}"
DEFINES += "VERSION=\\\"$${VERSION}\\\""
