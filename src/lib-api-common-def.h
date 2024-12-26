#ifndef LIB_API_COMMON_DEF_H
#define LIB_API_COMMON_DEF_H

#include <QtGlobal>

#ifdef Q_OS_WIN
#define CALL_CONV __stdcall
#else
#define CALL_CONV
#endif

#endif
