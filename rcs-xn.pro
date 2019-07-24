TARGET = rcs-xn
TEMPLATE = lib
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += RCS_XN_SHARED_LIBRARY

SOURCES += \
	src/rcs-xn.cpp \
	src/settings.cpp \
	src/signals.cpp
HEADERS += \
	src/rcs-xn.h \
	src/errors.h \
	src/events.h \
	src/settings.h \
	src/util.h \
	src/signals.h

SOURCES += \
	lib/xn-lib-cpp-qt/xn.cpp
HEADERS += \
	lib/xn-lib-cpp-qt/q-str-exception.h \
	lib/xn-lib-cpp-qt/xn.h \
	lib/xn-lib-cpp-qt/xn-typedefs.h \
	lib/xn-lib-cpp-qt/xn-commands.h \
	lib/xn-lib-cpp-qt/xn-loco-addr.h

CONFIG += c++14 dll
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic -enable-stdcall-fixup

win32 {
	QMAKE_LFLAGS += -Wl,--kill-at
}
win64 {
	QMAKE_LFLAGS += -Wl,--kill-at
}

QT += serialport

VERSION_MAJOR = 1
VERSION_MINOR = 0

DEFINES += "VERSION_MAJOR=$$VERSION_MAJOR" \
	"VERSION_MINOR=$$VERSION_MINOR"

#Target version
VERSION = "$${VERSION_MAJOR}.$${VERSION_MINOR}"
DEFINES += "VERSION=\\\"$${VERSION}\\\""
