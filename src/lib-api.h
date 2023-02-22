#ifndef LIB_API_H
#define LIB_API_H

/* This file deafines prototypes of library API functions. */

#include <QtCore/QtGlobal>

#include "lib-api-common-def.h"
#include "events.h"

namespace RcsXn {

extern "C" {
Q_DECL_EXPORT int CALL_CONV LoadConfig(char16_t *filename);
Q_DECL_EXPORT int CALL_CONV SaveConfig(char16_t *filename);
Q_DECL_EXPORT void CALL_CONV SetConfigFileName(char16_t *filename);

Q_DECL_EXPORT void CALL_CONV SetLogLevel(unsigned int loglevel);
Q_DECL_EXPORT unsigned int CALL_CONV GetLogLevel();

Q_DECL_EXPORT void CALL_CONV ShowConfigDialog();
Q_DECL_EXPORT void CALL_CONV HideConfigDialog();

Q_DECL_EXPORT int CALL_CONV Open();
Q_DECL_EXPORT int CALL_CONV OpenDevice(char16_t *device, bool persist);
Q_DECL_EXPORT int CALL_CONV Close();
Q_DECL_EXPORT bool CALL_CONV Opened();

Q_DECL_EXPORT int CALL_CONV Start();
Q_DECL_EXPORT int CALL_CONV Stop();
Q_DECL_EXPORT bool CALL_CONV Started();

Q_DECL_EXPORT int CALL_CONV GetInput(unsigned int module, unsigned int port);
Q_DECL_EXPORT int CALL_CONV GetOutput(unsigned int module, unsigned int port);
Q_DECL_EXPORT int CALL_CONV SetOutput(unsigned int module, unsigned int port, int state);
Q_DECL_EXPORT int CALL_CONV GetInputType(unsigned int module, unsigned int port);
Q_DECL_EXPORT int CALL_CONV GetOutputType(unsigned int module, unsigned int port);

Q_DECL_EXPORT int CALL_CONV SetInput(unsigned int module, unsigned int port, int state);
Q_DECL_EXPORT bool CALL_CONV IsSimulation();

Q_DECL_EXPORT int CALL_CONV GetDeviceCount();
Q_DECL_EXPORT void CALL_CONV GetDeviceSerial(int index, char16_t *serial,
                                                    unsigned int serialLen);

Q_DECL_EXPORT unsigned int CALL_CONV GetModuleCount();
Q_DECL_EXPORT unsigned int CALL_CONV GetMaxModuleAddr();
Q_DECL_EXPORT bool CALL_CONV IsModule(unsigned int module);
Q_DECL_EXPORT bool CALL_CONV IsModuleFailure(unsigned int module);
Q_DECL_EXPORT int CALL_CONV GetModuleTypeStr(unsigned int module, char16_t *type,
                                                    unsigned int typeLen);
Q_DECL_EXPORT int CALL_CONV GetModuleName(unsigned int module, char16_t *name,
                                                 unsigned int nameLen);
Q_DECL_EXPORT int CALL_CONV GetModuleFW(unsigned int module, char16_t *fw,
                                               unsigned int fwLen);
Q_DECL_EXPORT unsigned int CALL_CONV GetModuleInputsCount(unsigned int module);
Q_DECL_EXPORT unsigned int CALL_CONV GetModuleOutputsCount(unsigned int module);

Q_DECL_EXPORT bool CALL_CONV ApiSupportsVersion(unsigned int version);
Q_DECL_EXPORT int CALL_CONV ApiSetVersion(unsigned int version);
Q_DECL_EXPORT unsigned int CALL_CONV GetDeviceVersion(char16_t *version,
                                                             unsigned int versionLen);
Q_DECL_EXPORT unsigned int CALL_CONV GetDriverVersion(char16_t *version,
                                                             unsigned int versionLen);

Q_DECL_EXPORT void CALL_CONV BindBeforeOpen(StdNotifyEvent f, void *data);
Q_DECL_EXPORT void CALL_CONV BindAfterOpen(StdNotifyEvent f, void *data);
Q_DECL_EXPORT void CALL_CONV BindBeforeClose(StdNotifyEvent f, void *data);
Q_DECL_EXPORT void CALL_CONV BindAfterClose(StdNotifyEvent f, void *data);

Q_DECL_EXPORT void CALL_CONV BindBeforeStart(StdNotifyEvent f, void *data);
Q_DECL_EXPORT void CALL_CONV BindAfterStart(StdNotifyEvent f, void *data);
Q_DECL_EXPORT void CALL_CONV BindBeforeStop(StdNotifyEvent f, void *data);
Q_DECL_EXPORT void CALL_CONV BindAfterStop(StdNotifyEvent f, void *data);

Q_DECL_EXPORT void CALL_CONV BindOnError(StdErrorEvent f, void *data);
Q_DECL_EXPORT void CALL_CONV BindOnLog(StdLogEvent f, void *data);
Q_DECL_EXPORT void CALL_CONV BindOnInputChanged(StdModuleChangeEvent f, void *data);
Q_DECL_EXPORT void CALL_CONV BindOnOutputChanged(StdModuleChangeEvent f, void *data);
Q_DECL_EXPORT void CALL_CONV BindOnScanned(StdNotifyEvent f, void *data);
}

} // namespace RcsXn

#endif
