TARGET = rcs-xn
TEMPLATE = lib
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += RCS_XN_SHARED_LIBRARY

SOURCES += src/rcs-xn.cpp
HEADERS += src/rcs-xn.h src/errors.h

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

QT += serialport

VERSION_MAJOR = 1
VERSION_MINOR = 0

DEFINES += "VERSION_MAJOR=$$VERSION_MAJOR" \
	"VERSION_MINOR=$$VERSION_MINOR"

#Target version
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}
