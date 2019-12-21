#ifndef LIB_API_H
#define LIB_API_H

/* This file deafines prototypes of library API functions. */

#include <QtCore/QtGlobal>

#include "lib-api-common-def.h"
#include "events.h"

namespace RcsXn {

extern "C" {
RCS_XN_SHARED_EXPORT int CALL_CONV LoadConfig(char16_t *filename);
RCS_XN_SHARED_EXPORT int CALL_CONV SaveConfig(char16_t *filename);
RCS_XN_SHARED_EXPORT void CALL_CONV SetConfigFileName(char16_t *filename);

RCS_XN_SHARED_EXPORT void CALL_CONV SetLogLevel(unsigned int loglevel);
RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetLogLevel();

RCS_XN_SHARED_EXPORT void CALL_CONV ShowConfigDialog();
RCS_XN_SHARED_EXPORT void CALL_CONV HideConfigDialog();

RCS_XN_SHARED_EXPORT int CALL_CONV Open();
RCS_XN_SHARED_EXPORT int CALL_CONV OpenDevice(char16_t *device, bool persist);
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

RCS_XN_SHARED_EXPORT int CALL_CONV SetInput(unsigned int module, unsigned int port, int state);
RCS_XN_SHARED_EXPORT bool CALL_CONV IsSimulation();

RCS_XN_SHARED_EXPORT int CALL_CONV GetDeviceCount();
RCS_XN_SHARED_EXPORT void CALL_CONV GetDeviceSerial(int index, char16_t *serial,
                                                    unsigned int serialLen);

RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetModuleCount();
RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetMaxModuleAddr();
RCS_XN_SHARED_EXPORT bool CALL_CONV IsModule(unsigned int module);
RCS_XN_SHARED_EXPORT bool CALL_CONV IsModuleFailure(unsigned int module);
RCS_XN_SHARED_EXPORT int CALL_CONV GetModuleTypeStr(unsigned int module, char16_t *type,
                                                    unsigned int typeLen);
RCS_XN_SHARED_EXPORT int CALL_CONV GetModuleName(unsigned int module, char16_t *name,
                                                 unsigned int nameLen);
RCS_XN_SHARED_EXPORT int CALL_CONV GetModuleFW(unsigned int module, char16_t *fw,
                                               unsigned int fwLen);
RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetModuleInputsCount(unsigned int module);
RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetModuleOutputsCount(unsigned int module);

RCS_XN_SHARED_EXPORT bool CALL_CONV ApiSupportsVersion(unsigned int version);
RCS_XN_SHARED_EXPORT int CALL_CONV ApiSetVersion(unsigned int version);
RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetDeviceVersion(char16_t *version,
                                                             unsigned int versionLen);
RCS_XN_SHARED_EXPORT unsigned int CALL_CONV GetDriverVersion(char16_t *version,
                                                             unsigned int versionLen);

RCS_XN_SHARED_EXPORT void CALL_CONV BindBeforeOpen(StdNotifyEvent f, void *data);
RCS_XN_SHARED_EXPORT void CALL_CONV BindAfterOpen(StdNotifyEvent f, void *data);
RCS_XN_SHARED_EXPORT void CALL_CONV BindBeforeClose(StdNotifyEvent f, void *data);
RCS_XN_SHARED_EXPORT void CALL_CONV BindAfterClose(StdNotifyEvent f, void *data);

RCS_XN_SHARED_EXPORT void CALL_CONV BindBeforeStart(StdNotifyEvent f, void *data);
RCS_XN_SHARED_EXPORT void CALL_CONV BindAfterStart(StdNotifyEvent f, void *data);
RCS_XN_SHARED_EXPORT void CALL_CONV BindBeforeStop(StdNotifyEvent f, void *data);
RCS_XN_SHARED_EXPORT void CALL_CONV BindAfterStop(StdNotifyEvent f, void *data);

RCS_XN_SHARED_EXPORT void CALL_CONV BindOnError(StdErrorEvent f, void *data);
RCS_XN_SHARED_EXPORT void CALL_CONV BindOnLog(StdLogEvent f, void *data);
RCS_XN_SHARED_EXPORT void CALL_CONV BindOnInputChanged(StdModuleChangeEvent f, void *data);
RCS_XN_SHARED_EXPORT void CALL_CONV BindOnOutputChanged(StdModuleChangeEvent f, void *data);
RCS_XN_SHARED_EXPORT void CALL_CONV BindOnScanned(StdNotifyEvent f, void *data);
}

} // namespace RcsXn

#endif
