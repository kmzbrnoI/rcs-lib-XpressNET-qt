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
	llErrors = 1,
	llStateChange = 2,
	llCommands = 3,
	llRawCommands = 4,
	llDebug = 5,
};

extern "C" {
	RCS_XN_SHARED_EXPORT int CALL_CONV LoadConfig(char* filename);
	RCS_XN_SHARED_EXPORT int CALL_CONV SaveConfig(char* filename);

	RCS_XN_SHARED_EXPORT void CALL_CONV SetLogLevel(unsigned int loglevel);
	RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetLogLevel();

	RCS_XN_SHARED_EXPORT void CALL_CONV ShowConfigDialog();
	RCS_XN_SHARED_EXPORT void CALL_CONV HideConfigDialog();

	RCS_XN_SHARED_EXPORT int CALL_CONV Open();
	RCS_XN_SHARED_EXPORT int CALL_CONV OpenDevice(char* device, bool persist);
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

	RCS_XN_SHARED_EXPORT void CALL_CONV GetDeviceSerial(int index, char* serial,
	                                                    unsigned int serialLen);

	RCS_XN_SHARED_EXPORT bool CALL_CONV IsModule(unsigned int module);
	RCS_XN_SHARED_EXPORT bool CALL_CONV IsModuleFailure(unsigned int module);
	RCS_XN_SHARED_EXPORT int CALL_CONV GetModuleTypeStr(char* type, unsigned int typeLen);
	RCS_XN_SHARED_EXPORT int CALL_CONV GetModuleName(unsigned int module, char* name,
	                                                 unsigned int nameLen);
	RCS_XN_SHARED_EXPORT int CALL_CONV GetModuleFW(unsigned int module, char* fw, unsigned int fwLen);
	RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetModuleInputsCount(unsigned int module);
	RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetModuleOutputsCount(unsigned int module);

	RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetDeviceVersion(char* version, unsigned int versionLen);
	RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetDriverVersion(char* version, unsigned int versionLen);

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

	explicit RcsXn(QObject *parent = nullptr);
	virtual ~RcsXn();

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
