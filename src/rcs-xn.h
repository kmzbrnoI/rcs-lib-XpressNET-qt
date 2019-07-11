#ifndef RCS_XN_H
#define RCS_XN_H

// TODO Add docstring

#define RCS_XN_VERSION_MAJOR 1
#define RCS_XN_VERSION_MINOR 0

#if defined(RCS_XN_SHARED_LIBRARY)
#  define RCS_XN_SHARED_EXPORT Q_DECL_EXPORT
#else
#  define RCS_XN_SHARED_EXPORT Q_DECL_IMPORT
#endif

#ifdef Q_OS_WIN
#  define CALL_CONV __stdcall
#else
#  define CALL_CONV
#endif

#include <QObject>
#include <QtCore/QtGlobal>

#include "events.h"
#include "lib/xn-lib-cpp-qt/xn.h"
#include "settings.h"

namespace RcsXn {

constexpr char CONFIG_FN[] = "rcsXn.ini";

enum class RcsXnLogLevel {
	llNo = 0,
	llError = 1,
	llWarning = 2,
	llInfo = 3,
	llRawCommands = 4,
	llDebug = 5,
};

enum class RcsStartState {
	stopped = 0,
	scanning = 1,
	started = 2,
};

extern "C" {
	RCS_XN_SHARED_EXPORT int CALL_CONV LoadConfig(char16_t* filename);
	RCS_XN_SHARED_EXPORT int CALL_CONV SaveConfig(char16_t* filename);

	RCS_XN_SHARED_EXPORT void CALL_CONV SetLogLevel(unsigned int loglevel);
	RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetLogLevel();

	RCS_XN_SHARED_EXPORT void CALL_CONV ShowConfigDialog();
	RCS_XN_SHARED_EXPORT void CALL_CONV HideConfigDialog();

	RCS_XN_SHARED_EXPORT int CALL_CONV Open();
	RCS_XN_SHARED_EXPORT int CALL_CONV OpenDevice(char16_t* device, bool persist);
	RCS_XN_SHARED_EXPORT int CALL_CONV Close();
	RCS_XN_SHARED_EXPORT bool CALL_CONV Opened();

	RCS_XN_SHARED_EXPORT int CALL_CONV Start();
	RCS_XN_SHARED_EXPORT int CALL_CONV Stop();
	RCS_XN_SHARED_EXPORT bool CALL_CONV Started();

	RCS_XN_SHARED_EXPORT int CALL_CONV GetInput(unsigned int module, unsigned int port);
	RCS_XN_SHARED_EXPORT int CALL_CONV GetOutput(unsigned int module, unsigned int port);
	RCS_XN_SHARED_EXPORT int CALL_CONV SetOutput(unsigned int module, unsigned int port, int state);
	RCS_XN_SHARED_EXPORT int CALL_CONV GetInputType(unsigned int module, unsigned int port);
	RCS_XN_SHARED_EXPORT int CALL_CONV GetOutputType(unsigned int module, unsigned int port);

	RCS_XN_SHARED_EXPORT void CALL_CONV GetDeviceSerial(int index, char16_t* serial,
	                                                    unsigned int serialLen);

	RCS_XN_SHARED_EXPORT bool CALL_CONV IsModule(unsigned int module);
	RCS_XN_SHARED_EXPORT bool CALL_CONV IsModuleFailure(unsigned int module);
	RCS_XN_SHARED_EXPORT int CALL_CONV GetModuleTypeStr(char16_t* type, unsigned int typeLen);
	RCS_XN_SHARED_EXPORT int CALL_CONV GetModuleName(unsigned int module, char16_t* name,
	                                                 unsigned int nameLen);
	RCS_XN_SHARED_EXPORT int CALL_CONV GetModuleFW(unsigned int module, char16_t* fw, unsigned int fwLen);
	RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetModuleInputsCount(unsigned int module);
	RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetModuleOutputsCount(unsigned int module);

	RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetDeviceVersion(char16_t* version, unsigned int versionLen);
	RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetDriverVersion(char16_t* version, unsigned int versionLen);

	RCS_XN_SHARED_EXPORT void CALL_CONV BindBeforeOpen(StdNotifyEvent f, void* data);
	RCS_XN_SHARED_EXPORT void CALL_CONV BindAfterOpen(StdNotifyEvent f, void* data);
	RCS_XN_SHARED_EXPORT void CALL_CONV BindBeforeClose(StdNotifyEvent f, void* data);
	RCS_XN_SHARED_EXPORT void CALL_CONV BindAfterClose(StdNotifyEvent f, void* data);

	RCS_XN_SHARED_EXPORT void CALL_CONV BindBeforeStart(StdNotifyEvent f, void* data);
	RCS_XN_SHARED_EXPORT void CALL_CONV BindAfterStart(StdNotifyEvent f, void* data);
	RCS_XN_SHARED_EXPORT void CALL_CONV BindBeforeStop(StdNotifyEvent f, void* data);
	RCS_XN_SHARED_EXPORT void CALL_CONV BindAfterStop(StdNotifyEvent f, void* data);

	RCS_XN_SHARED_EXPORT void CALL_CONV BindOnError(StdErrorEvent f, void* data);
	RCS_XN_SHARED_EXPORT void CALL_CONV BindOnLog(StdLogEvent f, void* data);
	RCS_XN_SHARED_EXPORT void CALL_CONV BindOnInputChanged(StdModuleChangeEvent f, void* data);
	RCS_XN_SHARED_EXPORT void CALL_CONV BindOnOutputChanged(StdModuleChangeEvent f, void* data);
	RCS_XN_SHARED_EXPORT void CALL_CONV BindOnScanned(StdNotifyEvent f, void* data);
}

class RcsXn : public QObject {
	Q_OBJECT

public:
	RcsEvents events;
	Xn::XpressNet xn;
	Settings s;
	RcsXnLogLevel loglevel;
	RcsStartState started = RcsStartState::stopped;

	explicit RcsXn(QObject *parent = nullptr);
	virtual ~RcsXn();

	void log(const QString& msg, RcsXnLogLevel loglevel);
	void error(const QString& message, uint16_t code, unsigned int module);
	void error(const QString& message, uint16_t code);
	void error(const QString& message);
	void first_scan();

	int openDevice(const QString& device, bool persist);
	int close();
	void loadConfig(const QString& filename);

private slots:
	void xnOnError(QString error);
	void xnOnLog(QString message, Xn::XnLogLevel loglevel);
	void xnOnConnect();
	void xnOnDisconnect();
	void xnOnTrkStatusChanged(Xn::XnTrkStatus);
	void xnOnAccInputChanged(uint8_t groupAddr, bool nibble, bool error, Xn::XnFeedbackType inputType,
	                         Xn::XnAccInputsState state);

signals:

private:

};

extern RcsXn rx;

} // namespace RcsXn

#endif
