#include "rcs-xn.h"
#include "errors.h"

namespace RcsXn {

RcsXn rx;

///////////////////////////////////////////////////////////////////////////////

RcsXn::RcsXn(QObject *parent)
	: QObject(parent), xn(this) {
	QObject::connect(&xn, SIGNAL(onError()), this, SLOT(onOnError()));
	QObject::connect(&xn, SIGNAL(onLog()), this, SLOT(onOnLog()));
	QObject::connect(&xn, SIGNAL(onConnect()), this, SLOT(onOnConnect()));
	QObject::connect(&xn, SIGNAL(onDisconnect()), this, SLOT(onOnDisconnect()));
	QObject::connect(&xn, SIGNAL(onTrkStatusChanged()), this, SLOT(onOnTrkStatusChanged()));
	QObject::connect(&xn, SIGNAL(onAccInputChanged()), this, SLOT(onOnAccInputChanged()));

	s.load(CONFIG_FN);
}

RcsXn::~RcsXn() {
	s.save(CONFIG_FN); // optional
}

void RcsXn::log(const QString& msg, RcsXnLogLevel loglevel) {
	this->events.call(this->events.onLog, static_cast<int>(loglevel), msg);
}

void RcsXn::error(const QString& message, uint16_t code, unsigned int module) {
	this->events.call(this->events.onError, code, module, message);
}

void RcsXn::error(const QString& message, uint16_t code) {
	this->error(message, code, 0);
}

void RcsXn::error(const QString& message) {
	this->error(message, RCS_GENERAL_EXCEPTION, 0);
}

int RcsXn::openDevice(const QString& device, bool persist) {
	events.call(rx.events.beforeOpen);

	if (xn.connected())
		return RCS_ALREADY_OPENNED;

	log("Connecting to XN...", RcsXnLogLevel::llInfo);

	try {
		xn.connect(s["XN"]["port"].toString(), s["XN"]["baudrate"].toInt(),
		           static_cast<QSerialPort::FlowControl>(s["XN"]["flowcontrol"].toInt()));
	} catch (const Xn::QStrException& e) {
		error("XN connect error while opening serial port '" +
		      s["XN"]["port"].toString() + "':" + e, RCS_CANNOT_OPEN_PORT);
		return RCS_CANNOT_OPEN_PORT;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Xn events

void RcsXn::xnOnError(QString error) {
	this->error(error);
}

void RcsXn::xnOnLog(QString message, Xn::XnLogLevel loglevel) {
	// TODO: a little loglevel mismatch
	this->log(message, static_cast<RcsXnLogLevel>(loglevel));
}

void RcsXn::xnOnConnect() {
	this->events.call(this->events.afterOpen);
}

void RcsXn::xnOnDisconnect() {
	this->events.call(this->events.afterClose);
}

void RcsXn::xnOnTrkStatusChanged(Xn::XnTrkStatus) {
}

void RcsXn::xnOnAccInputChanged(uint8_t groupAddr, bool nibble, bool error,
                                Xn::XnFeedbackType inputType, Xn::XnAccInputsState state) {
}

///////////////////////////////////////////////////////////////////////////////
// Open/close

extern "C" RCS_XN_SHARED_EXPORT int CALL_CONV Open() {
	return rx.openDevice(rx.s["XN"]["port"].toString(), false);
}

extern "C" RCS_XN_SHARED_EXPORT int CALL_CONV OpenDevice(char16_t* device, bool persist) {
	return rx.openDevice(QString::fromUtf16(device), persist);
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
