#ifndef EVENTS_H
#define EVENTS_H

#include <QString>
#include <cstdint>

#include "lib-api-common-def.h"

/* This file provides storage & calling capabilities of callbacks from the
 * library back to the hJOPserver.
 */

namespace RcsXn {

using StdNotifyEvent = void CALL_CONV (*)(const void *sender, const void *data);
using StdLogEvent = void CALL_CONV (*)(const void *sender, const void *data, int loglevel,
                                       const uint16_t *msg);
using StdErrorEvent = void CALL_CONV (*)(const void *sender, const void *data, uint16_t errValue,
                                         unsigned int errAddr, const uint16_t *errMsg);
using StdModuleChangeEvent = void CALL_CONV (*)(const void *sender, const void *data,
                                                unsigned int module);

template <typename F>
struct EventData {
	F func = nullptr;
	void *data = nullptr;
	bool defined() const { return this->func != nullptr; }
};

struct RcsEvents {
	EventData<StdNotifyEvent> beforeOpen;
	EventData<StdNotifyEvent> afterOpen;
	EventData<StdNotifyEvent> beforeClose;
	EventData<StdNotifyEvent> afterClose;

	EventData<StdNotifyEvent> beforeStart;
	EventData<StdNotifyEvent> afterStart;
	EventData<StdNotifyEvent> beforeStop;
	EventData<StdNotifyEvent> afterStop;

	EventData<StdNotifyEvent> onScanned;
	EventData<StdErrorEvent> onError;
	EventData<StdLogEvent> onLog;
	EventData<StdModuleChangeEvent> onInputChanged;
	EventData<StdModuleChangeEvent> onOutputChanged;

	void call(const EventData<StdNotifyEvent> &e) const {
		if (e.defined())
			e.func(this, e.data);
	}
	void call(const EventData<StdErrorEvent> &e, uint16_t errValue, unsigned int errAddr,
	          const QString &errMsg) const {
		if (e.defined())
			e.func(this, e.data, errValue, errAddr, errMsg.utf16());
	}
	void call(const EventData<StdLogEvent> &e, int loglevel, const QString &msg) const {
		if (e.defined())
			e.func(this, e.data, loglevel, msg.utf16());
	}
	void call(const EventData<StdModuleChangeEvent> &e, unsigned int module) const {
		if (e.defined())
			e.func(this, e.data, module);
	}

	template <typename F>
	static void bind(EventData<F> &event, const F &func, void *const data) {
		event.func = func;
		event.data = data;
	}
};

} // namespace RcsXn

#endif
