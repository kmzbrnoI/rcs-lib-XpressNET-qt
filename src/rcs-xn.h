#ifndef RCS_XN_H
#define RCS_XN_H

// TODO Add docstring

#define RCS_XN_VERSION_MAJOR 1
#define RCS_XN_VERSION_MINOR 0

#include <QObject>
#include <QtCore/QtGlobal>

#if defined(RCS_XN_SHARED_LIBRARY)
#  define RCS_XN_SHARED_EXPORT Q_DECL_EXPORT
#else
#  define RCS_XN_SHARED_EXPORT Q_DECL_IMPORT
#endif

namespace RcsXn {

#ifdef Q_OS_WIN
#  define CALL_CONV __stdcall
#else
#  define CALL_CONV
#endif

extern "C" {
	RCS_XN_SHARED_EXPORT int CALL_CONV LoadConfig(char* filename);
	RCS_XN_SHARED_EXPORT int CALL_CONV SaveConfig(char* filename);
}

class RcsXn : public QObject {
	Q_OBJECT

public:

private slots:

signals:

private:

};

} // namespace RcsXn

#endif
