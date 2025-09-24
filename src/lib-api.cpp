#include "lib-api.h"
#include "errors.h"
#include "rcs-xn.h"
#include "util.h"

/* This file deafines all library exported API functions. */

namespace RcsXn {

unsigned int rcs_api_version = 0;

///////////////////////////////////////////////////////////////////////////////
// Open/close

int Open() {
	try {
		return rx.openDevice(rx.s["XN"]["port"].toString(), false);
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
	} catch (const QStrException& e) {
		rx.log(e.str(), RcsXnLogLevel::llError);
		return RCS_FILE_CANNOT_ACCESS;
	} catch (...) { return RCS_FILE_CANNOT_ACCESS; }
	return 0;
}

int SaveConfig(char16_t *filename) {
	try {
		rx.saveConfig(QString::fromUtf16(filename));
	} catch (const QStrException& e) {
		rx.log(e.str(), RcsXnLogLevel::llError);
		return RCS_FILE_CANNOT_ACCESS;
	} catch (...) { return RCS_FILE_CANNOT_ACCESS; }
	return 0;
}

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
	try {
		if (rx.started == RcsStartState::stopped)
			return RCS_NOT_STARTED;
		if ((module >= IO_IN_MODULES_COUNT) || (!rx.modules_in[module].realActive))
			return (rx.modules_in[module].wantActive) ? RCS_MODULE_FAILED : RCS_MODULE_INVALID_ADDR;
		if ((port > IO_IN_MODULE_PIN_COUNT) || (port == 0)) { // ports 1-8, not 0-7!
#ifdef IGNORE_PIN_BOUNDS
			return 0;
#else
			return RCS_PORT_INVALID_NUMBER;
#endif
		}
		if (rx.started == RcsStartState::scanning)
			return RCS_INPUT_NOT_YET_SCANNED;

		return ((rx.modules_in[module].state[port-1] == XnInState::on) ||
		        (rx.modules_in[module].state[port-1] == XnInState::falling));
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

int GetOutput(unsigned int module, unsigned int port) {
	try {
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
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

int SetOutput(unsigned int module, unsigned int port, int state) {
	try {
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

		if (rx.isSignal(portAddr))
			return rx.setSignal(static_cast<uint16_t>(portAddr), static_cast<unsigned int>(state));

		if ((rx.binary[module]) && (state == 0)) {
			portAddr = (module<<1) + ((!port)&1);
			state = 1;
		}
		if (rx.outputs[portAddr] == static_cast<bool>(state))
			return 0;
		return rx.setPlainOutput(portAddr, state, true);
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

bool IsSimulation() {
	try {
		return rx.s["global"]["mockInputs"].toBool();
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

int SetInput(unsigned int module, unsigned int port, int state) {
	try {
		// only debug method
		if (!rx.s["global"]["mockInputs"].toBool())
			return 0;
		if (rx.started == RcsStartState::stopped)
			return 0;
		if ((module >= IO_IN_MODULES_COUNT) || (!rx.modules_in[module].wantActive))
			return (rx.modules_in[module].wantActive) ? RCS_MODULE_FAILED : RCS_MODULE_INVALID_ADDR;
		if ((port > IO_IN_MODULE_PIN_COUNT) || (port == 0)) { // ports 1-8, not 0-7!
#ifdef IGNORE_PIN_BOUNDS
			return 0;
#else
			return RCS_PORT_INVALID_NUMBER;
#endif
		}

		rx.modules_in[module].state[port-1] = (state == 1) ? XnInState::on : XnInState::off;
		rx.events.call(rx.events.onInputChanged, module);
		return 0;
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

int GetInputType(unsigned int module, unsigned int port) {
	try {
		(void)module;
		(void)port;
		return 0; // all inputs are plain inputs
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

int GetOutputType(unsigned int module, unsigned int port) {
	try {
		unsigned int portAddr = (module<<1) + (port&1); // 0-2047
		return rx.isSignal(portAddr) ? 1 : 0;
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

///////////////////////////////////////////////////////////////////////////////
// Module questionaries

unsigned int GetModuleCount() { return rx.modules_count; }

bool IsModule(unsigned int module) {
	try {
		if (module < IO_IN_MODULES_COUNT && rx.modules_in[module].wantActive)
			return true;
		if (module < IO_OUT_MODULES_COUNT && rx.user_active_out[module])
			return true;
		return false;
	} catch (...) { return false; }
}

unsigned int GetMaxModuleAddr() { return std::max(IO_IN_MODULES_COUNT, IO_OUT_MODULES_COUNT) - 1; }

bool IsModuleFailure(unsigned int module) {
	try {
		// Output module is always marked as active - so outputs
		// could be set even if input module is absent.
		if (module < IO_OUT_MODULES_COUNT && rx.user_active_out[module])
			return false;
		return (rx.started == RcsStartState::started && module < IO_IN_MODULES_COUNT && rx.modules_in[module].wantActive && !rx.modules_in[module].realActive);
	} catch (...) { return false; }
}

int GetModuleTypeStr(unsigned int module, char16_t *type, unsigned int typeLen) {
	try {
		(void)module;
		const QString stype = "XN";
		StrUtil::strcpy<char16_t>(reinterpret_cast<const char16_t *>(stype.utf16()), type, typeLen);
		return 0;
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

int GetModuleName(unsigned int module, char16_t *name, unsigned int nameLen) {
	try {
		if (module >= std::max(IO_IN_MODULES_COUNT, IO_OUT_MODULES_COUNT))
			return RCS_MODULE_INVALID_ADDR;
		const QString str = rx.modules_in[module].name;
		StrUtil::strcpy<char16_t>(reinterpret_cast<const char16_t *>(str.utf16()), name, nameLen);
		return 0;
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

int GetModuleFW(unsigned int module, char16_t *fw, unsigned int fwLen) {
	try {
		if (module >= std::max(IO_IN_MODULES_COUNT, IO_OUT_MODULES_COUNT))
			return RCS_MODULE_INVALID_ADDR;
		const QString sfw = "-";
		StrUtil::strcpy<char16_t>(reinterpret_cast<const char16_t *>(sfw.utf16()), fw, fwLen);
		return 0;
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

unsigned int GetModuleInputsCount(unsigned int module) {
	try {
		if (module >= std::max(IO_IN_MODULES_COUNT, IO_OUT_MODULES_COUNT))
			return RCS_MODULE_INVALID_ADDR;
		if (module >= IO_IN_MODULES_COUNT)
			return 0; // intentionally not RCS_MODULE_INVALID_ADDR
		return rx.modules_in[module].wantActive ? IO_IN_MODULE_PIN_COUNT+1 : 0; // pin 0 ignored, indexing from 1
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

unsigned int GetModuleOutputsCount(unsigned int module) {
	try {
		if (module >= std::max(IO_IN_MODULES_COUNT, IO_OUT_MODULES_COUNT))
			return RCS_MODULE_INVALID_ADDR;
		if (module >= IO_OUT_MODULES_COUNT)
			return 0; // intentionally not RCS_MODULE_INVALID_ADDR
		if (!rx.user_active_out[module])
			return 0;
		if (rx.isSignal(module*IO_OUT_MODULE_PIN_COUNT))
			return 1; // signal -> just one output
		return IO_OUT_MODULE_PIN_COUNT;
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

bool IsModuleError(unsigned int module) {
	(void)module;
	return false;
}

bool IsModuleWarning(unsigned int module) {
	(void)module;
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Versions

bool ApiSupportsVersion(unsigned int version) {
	try {
		return std::find(API_SUPPORTED_VERSIONS.begin(), API_SUPPORTED_VERSIONS.end(), version) !=
			   API_SUPPORTED_VERSIONS.end();
	} catch (...) { return false; }
}

int ApiSetVersion(unsigned int version) {
	try {
		if (!ApiSupportsVersion(version))
			return RCS_UNSUPPORTED_API_VERSION;

		rcs_api_version = version;
		return 0;
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

unsigned int GetDeviceVersion(char16_t *version, unsigned int versionLen) {
	try {
		const QString sversion = "LI HW: " + QString::number(rx.li_ver_hw) + ", LI SW: " +
								  QString::number(rx.li_ver_sw);
		StrUtil::strcpy<char16_t>(reinterpret_cast<const char16_t *>(sversion.utf16()), version,
								  versionLen);
		return 0;
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
}

unsigned int GetDriverVersion(char16_t *version, unsigned int versionLen) {
	try {
		const QString sversion = VERSION;
		StrUtil::strcpy<char16_t>(reinterpret_cast<const char16_t *>(sversion.utf16()), version,
								  versionLen);
		return 0;
	} catch (...) { return RCS_GENERAL_EXCEPTION; }
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

void BindOnInputChanged(StdModuleChangeEvent f, void *data) { rx.events.bind(rx.events.onInputChanged, f, data); }
void BindOnOutputChanged(StdModuleChangeEvent f, void *data) { rx.events.bind(rx.events.onOutputChanged, f, data); }
void BindOnModuleChanged(StdModuleChangeEvent f, void *data) { rx.events.bind(rx.events.onModuleChanged, f, data); }

void BindOnScanned(StdNotifyEvent f, void *data) { rx.events.bind(rx.events.onScanned, f, data); }

///////////////////////////////////////////////////////////////////////////////

} // namespace RcsXn
