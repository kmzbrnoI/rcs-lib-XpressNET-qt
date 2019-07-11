#ifndef EVENTS_H
#define EVENTS_H

// TODO Add docstring

namespace RcsXn {

using StdNotifyEvent = void CALL_CONV (*)(void* sender, void* data);
using StdLogEvent = void CALL_CONV (*)(void* sender, void* data, int loglevel, const uint16_t* msg);
using StdErrorEvent = void CALL_CONV (*)(void* sender, void* data, uint16_t errValue, unsigned int errAddr, const uint16_t* errMsg);
using StdModuleChangeEvent = void CALL_CONV (*)(void* sender, void* data, unsigned int module);

template <typename F>
struct EventData {
	F func = nullptr;
	void* data = nullptr;
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

	void call(EventData<StdNotifyEvent> e) {
		if (e.defined())
			e.func(this, e.data);
	}
	void call(EventData<StdErrorEvent> e, uint16_t errValue, unsigned int errAddr, const QString& errMsg) {
		if (e.defined())
			e.func(this, e.data, errValue, errAddr, errMsg.utf16());
	}
	void call(EventData<StdLogEvent> e, int loglevel, const QString& msg) {
		if (e.defined())
			e.func(this, e.data, loglevel, msg.utf16());
	}
	void call(EventData<StdModuleChangeEvent> e, unsigned int module) {
		if (e.defined())
			e.func(this, e.data, module);
	}
};

} // namespace RcsXn

#endif
