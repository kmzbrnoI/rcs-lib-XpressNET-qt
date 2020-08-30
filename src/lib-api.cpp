#include "lib-api.h"
#include "errors.h"
#include "rcs-xn.h"
#include "util.h"

/* This file deafines all library exported API functions. */

namespace RcsXn {

///////////////////////////////////////////////////////////////////////////////
// Open/close

int Open() {
	try {
		return rx.openDevice(rx.s["XN"]["port"].toString(), false);
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

int OpenDevice(char16_t *device, bool persist) {
	try {
		return rx.openDevice(QString::fromUtf16(device), persist);
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

int Close() {
	try {
		return rx.close();
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

bool Opened() {
	try {
		return (rx.xn.connected() && (!rx.opening));
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

///////////////////////////////////////////////////////////////////////////////
// Start/stop

int Start() {
	try {
		return rx.start();
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

int Stop() {
	try {
		return rx.stop();
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

bool Started() {
	try {
		return (rx.started > RcsStartState::stopped);
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

///////////////////////////////////////////////////////////////////////////////
// Config

int LoadConfig(char16_t *filename) {
	if (rx.xn.connected())
		return RCS_FILE_DEVICE_OPENED;
	try {
		rx.config_filename = QString::fromUtf16(filename);
		rx.loadConfig(QString::fromUtf16(filename));
	} catch (...) { return RCS_FILE_CANNOT_ACCESS; }
	return 0;
}

int SaveConfig(char16_t *filename) {
	try {
		rx.saveConfig(QString::fromUtf16(filename));
	} catch (...) { return RCS_FILE_CANNOT_ACCESS; }
	return 0;
}

void SetConfigFileName(char16_t *filename) { rx.config_filename = QString::fromUtf16(filename); }

///////////////////////////////////////////////////////////////////////////////
// Loglevel

void SetLogLevel(unsigned int loglevel) {
	try {
		rx.setLogLevel(static_cast<RcsXnLogLevel>(loglevel));
	} catch (...) {}
}

unsigned int GetLogLevel() { return static_cast<unsigned int>(rx.loglevel); }

///////////////////////////////////////////////////////////////////////////////
// UI

void ShowConfigDialog() {
	try {
		rx.form.show();
	} catch (...) {}
}

void HideConfigDialog() {
	try {
		rx.form.close();
	} catch (...) {}
}

///////////////////////////////////////////////////////////////////////////////
// RCS IO

int GetInput(unsigned int module, unsigned int port) {
	if (rx.started == RcsStartState::stopped)
		return RCS_NOT_STARTED;
	if ((module >= IO_IN_MODULES_COUNT) || (!rx.real_active_in[module]))
		return RCS_MODULE_INVALID_ADDR;
	if ((port > IO_IN_MODULE_PIN_COUNT) || (port == 0)) { // ports 1-8, not 0-7!
#ifdef IGNORE_PIN_BOUNDS
		return 0;
#else
		return RCS_PORT_INVALID_NUMBER;
#endif
	}
	if (rx.started == RcsStartState::scanning)
		return RCS_INPUT_NOT_YET_SCANNED;

	unsigned int portAddr = module*IO_IN_MODULE_PIN_COUNT + port-1; // 0-2047
	return rx.inputs[portAddr];
}

int GetOutput(unsigned int module, unsigned int port) {
	if (rx.started == RcsStartState::stopped)
		return RCS_NOT_STARTED;
	if ((module >= IO_OUT_MODULES_COUNT) || (!rx.user_active_out[module]))
		return RCS_MODULE_INVALID_ADDR;
	if (port >= IO_OUT_MODULE_PIN_COUNT) {
#ifdef IGNORE_PIN_BOUNDS
		return 0;
#else
		return RCS_PORT_INVALID_NUMBER;
#endif
	}

	unsigned int portAddr = (module<<1) + (port&1); // 0-2047
	if (rx.isSignal(portAddr)) {
		const XnSignal &sig = rx.sig.at(portAddr/2);
		return static_cast<int>(sig.currentCode);
	}
	return rx.outputs[portAddr];
}

int SetOutput(unsigned int module, unsigned int port, int state) {
	if (rx.started == RcsStartState::stopped)
		return RCS_NOT_STARTED;
	if ((module >= IO_OUT_MODULES_COUNT) || (!rx.user_active_out[module]))
		return RCS_MODULE_INVALID_ADDR;
	if (port >= IO_OUT_MODULE_PIN_COUNT) {
#ifdef IGNORE_PIN_BOUNDS
		return 0;
#else
		return RCS_PORT_INVALID_NUMBER;
#endif
	}

	try {
		unsigned int portAddr = (module<<1) + (port&1); // 0-2047

		if (rx.isSignal(portAddr))
			return rx.setSignal(static_cast<uint16_t>(portAddr), static_cast<unsigned int>(state));

		if (rx.outputs[portAddr] == static_cast<bool>(state))
			return 0;
		return rx.setPlainOutput(portAddr, state);
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

bool IsSimulation() {
	return rx.s["global"]["mockInputs"].toBool();
}

int SetInput(unsigned int module, unsigned int port, int state) {
	// only debug method
	if (!rx.s["global"]["mockInputs"].toBool())
		return 0;
	if (rx.started == RcsStartState::stopped)
		return 0;
	if ((module >= IO_IN_MODULES_COUNT) || (!rx.user_active_in[module]))
		return RCS_MODULE_INVALID_ADDR;
	if ((port > IO_IN_MODULE_PIN_COUNT) || (port == 0)) { // ports 1-8, not 0-7!
#ifdef IGNORE_PIN_BOUNDS
		return 0;
#else
		return RCS_PORT_INVALID_NUMBER;
#endif
	}

	unsigned int portAddr = module*IO_IN_MODULE_PIN_COUNT + port-1; // 0-2047
	rx.inputs[portAddr] = static_cast<bool>(state);
	rx.events.call(rx.events.onInputChanged, module);
	return 0;
}

int GetInputType(unsigned int module, unsigned int port) {
	(void)module;
	(void)port;
	return 0; // all inputs are plain inputs
}

int GetOutputType(unsigned int module, unsigned int port) {
	unsigned int portAddr = (module<<1) + (port&1); // 0-2047
	return rx.isSignal(portAddr) ? 1 : 0;
}

///////////////////////////////////////////////////////////////////////////////
// Devices

int GetDeviceCount() { return 1; }

void GetDeviceSerial(int index, char16_t *serial, unsigned int serialLen) {
	(void)index;
	const QString sname = "COM port";
	StrUtil::strcpy<char16_t>(reinterpret_cast<const char16_t *>(sname.utf16()), serial, serialLen);
}

///////////////////////////////////////////////////////////////////////////////
// Module questionaries

unsigned int GetModuleCount() { return rx.modules_count; }

bool IsModule(unsigned int module) {
	if (module < IO_IN_MODULES_COUNT && rx.user_active_in[module])
		return true;
	if (module < IO_OUT_MODULES_COUNT && rx.user_active_out[module])
		return true;
	return false;
}

unsigned int GetMaxModuleAddr() { return std::max(IO_IN_MODULES_COUNT, IO_OUT_MODULES_COUNT) - 1; }

bool IsModuleFailure(unsigned int module) {
	return (rx.started == RcsStartState::started && module < IO_IN_MODULES_COUNT &&
	        rx.user_active_in[module] && !rx.real_active_in[module]);
}

int GetModuleTypeStr(unsigned int module, char16_t *type, unsigned int typeLen) {
	(void)module;
	const QString stype = "XN";
	StrUtil::strcpy<char16_t>(reinterpret_cast<const char16_t *>(stype.utf16()), type, typeLen);
	return 0;
}

int GetModuleName(unsigned int module, char16_t *name, unsigned int nameLen) {
	if (module >= std::max(IO_IN_MODULES_COUNT, IO_OUT_MODULES_COUNT))
		return RCS_MODULE_INVALID_ADDR;
	const QString str = "Module " + QString::number(module);
	StrUtil::strcpy<char16_t>(reinterpret_cast<const char16_t *>(str.utf16()), name, nameLen);
	return 0;
}

int GetModuleFW(unsigned int module, char16_t *fw, unsigned int fwLen) {
	if (module >= std::max(IO_IN_MODULES_COUNT, IO_OUT_MODULES_COUNT))
		return RCS_MODULE_INVALID_ADDR;
	const QString sfw = "-";
	StrUtil::strcpy<char16_t>(reinterpret_cast<const char16_t *>(sfw.utf16()), fw, fwLen);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Versions

bool ApiSupportsVersion(unsigned int version) {
	return std::find(API_SUPPORTED_VERSIONS.begin(), API_SUPPORTED_VERSIONS.end(), version) !=
	       API_SUPPORTED_VERSIONS.end();
}

int ApiSetVersion(unsigned int version) {
	if (!ApiSupportsVersion(version))
		return RCS_UNSUPPORTED_API_VERSION;

	rx.api_version = version;
	return 0;
}

unsigned int GetDeviceVersion(char16_t *version, unsigned int versionLen) {
	const QString sversion = "LI HW: " + QString::number(rx.li_ver_hw) + ", LI SW: " +
	                          QString::number(rx.li_ver_sw);
	StrUtil::strcpy<char16_t>(reinterpret_cast<const char16_t *>(sversion.utf16()), version,
	                          versionLen);
	return 0;
}

unsigned int GetDriverVersion(char16_t *version, unsigned int versionLen) {
	const QString sversion = VERSION;
	StrUtil::strcpy<char16_t>(reinterpret_cast<const char16_t *>(sversion.utf16()), version,
	                          versionLen);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// General library configuration

unsigned int GetModuleInputsCount(unsigned int module) {
	if (module >= IO_IN_MODULES_COUNT)
		return RCS_MODULE_INVALID_ADDR;
	return rx.user_active_in[module] ? IO_IN_MODULE_PIN_COUNT+1 : 0; // pin 0 ignored, indexing from 1
}

unsigned int GetModuleOutputsCount(unsigned int module) {
	if (module >= IO_OUT_MODULES_COUNT)
		return RCS_MODULE_INVALID_ADDR;
	if (!rx.user_active_out[module])
		return 0;
	if (rx.isSignal(module*IO_OUT_MODULE_PIN_COUNT))
		return 1; // signal -> just one output
	return IO_OUT_MODULE_PIN_COUNT;
}

///////////////////////////////////////////////////////////////////////////////
// Events binders

void BindBeforeOpen(StdNotifyEvent f, void *data) { rx.events.bind(rx.events.beforeOpen, f, data); }
void BindAfterOpen(StdNotifyEvent f, void *data) { rx.events.bind(rx.events.afterOpen, f, data); }
void BindBeforeClose(StdNotifyEvent f, void *data) {
	rx.events.bind(rx.events.beforeClose, f, data);
}
void BindAfterClose(StdNotifyEvent f, void *data) { rx.events.bind(rx.events.afterClose, f, data); }
void BindBeforeStart(StdNotifyEvent f, void *data) {
	rx.events.bind(rx.events.beforeStart, f, data);
}

void BindAfterStart(StdNotifyEvent f, void *data) { rx.events.bind(rx.events.afterStart, f, data); }
void BindBeforeStop(StdNotifyEvent f, void *data) { rx.events.bind(rx.events.beforeStop, f, data); }
void BindAfterStop(StdNotifyEvent f, void *data) { rx.events.bind(rx.events.afterStop, f, data); }
void BindOnError(StdErrorEvent f, void *data) { rx.events.bind(rx.events.onError, f, data); }
void BindOnLog(StdLogEvent f, void *data) { rx.events.bind(rx.events.onLog, f, data); }

void BindOnInputChanged(StdModuleChangeEvent f, void *data) {
	rx.events.bind(rx.events.onInputChanged, f, data);
}
void BindOnOutputChanged(StdModuleChangeEvent f, void *data) {
	rx.events.bind(rx.events.onOutputChanged, f, data);
}
void BindOnScanned(StdNotifyEvent f, void *data) { rx.events.bind(rx.events.onScanned, f, data); }

///////////////////////////////////////////////////////////////////////////////

} // namespace RcsXn
