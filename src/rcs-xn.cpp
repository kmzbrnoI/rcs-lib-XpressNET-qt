#include "rcs-xn.h"

namespace RcsXn {

RcsXn rx;

///////////////////////////////////////////////////////////////////////////////

RcsXn::RcsXn(QObject *parent)
	: QObject(parent), xn(this) {
}

///////////////////////////////////////////////////////////////////////////////
// Open/close

extern "C" RCS_XN_SHARED_EXPORT int CALL_CONV Open() {
	rx.events.call(rx.events.beforeOpen);
}

extern "C" RCS_XN_SHARED_EXPORT int CALL_CONV OpenDevice(char* device, bool persist) {
	rx.events.call(rx.events.beforeOpen);
}

extern "C" RCS_XN_SHARED_EXPORT int CALL_CONV Close() {
	rx.events.call(rx.events.beforeClose);
}

extern "C" RCS_XN_SHARED_EXPORT bool CALL_CONV Opened() {
	return rx.xn.connected();
}

///////////////////////////////////////////////////////////////////////////////
// Events binders

extern "C" RCS_XN_SHARED_EXPORT void CALL_CONV BindBeforeOpen(StdNotifyEvent f, void* data) {
	rx.events.bind(rx.events.beforeOpen, f, data);
}

extern "C" RCS_XN_SHARED_EXPORT void CALL_CONV BindAfterOpen(StdNotifyEvent f, void* data) {
	rx.events.bind(rx.events.afterOpen, f, data);
}

extern "C" RCS_XN_SHARED_EXPORT void CALL_CONV BindBeforeClose(StdNotifyEvent f, void* data) {
	rx.events.bind(rx.events.beforeClose, f, data);
}

extern "C" RCS_XN_SHARED_EXPORT void CALL_CONV BindAfterClose(StdNotifyEvent f, void* data) {
	rx.events.bind(rx.events.afterClose, f, data);
}

extern "C" RCS_XN_SHARED_EXPORT void CALL_CONV BindBeforeStart(StdNotifyEvent f, void* data) {
	rx.events.bind(rx.events.beforeStart, f, data);
}

extern "C" RCS_XN_SHARED_EXPORT void CALL_CONV BindAfterStart(StdNotifyEvent f, void* data) {
	rx.events.bind(rx.events.afterStart, f, data);
}

extern "C" RCS_XN_SHARED_EXPORT void CALL_CONV BindBeforeStop(StdNotifyEvent f, void* data) {
	rx.events.bind(rx.events.beforeStop, f, data);
}

extern "C" RCS_XN_SHARED_EXPORT void CALL_CONV BindAfterStop(StdNotifyEvent f, void* data) {
	rx.events.bind(rx.events.afterStop, f, data);
}

extern "C" RCS_XN_SHARED_EXPORT void CALL_CONV BindOnError(StdErrorEvent f, void* data) {
	rx.events.bind(rx.events.onError, f, data);
}

extern "C" RCS_XN_SHARED_EXPORT void CALL_CONV BindOnLog(StdLogEvent f, void* data) {
	rx.events.bind(rx.events.onLog, f, data);
}

extern "C" RCS_XN_SHARED_EXPORT void CALL_CONV BindOnInputChanged(StdModuleChangeEvent f, void* data) {
	rx.events.bind(rx.events.onInputChanged, f, data);
}

extern "C" RCS_XN_SHARED_EXPORT void CALL_CONV BindOnOutputChanged(StdModuleChangeEvent f, void* data) {
	rx.events.bind(rx.events.onOutputChanged, f, data);
}

extern "C" RCS_XN_SHARED_EXPORT void CALL_CONV BindOnScanned(StdNotifyEvent f, void* data) {
	rx.events.bind(rx.events.onScanned, f, data);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace RcsXn
