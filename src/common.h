#ifndef COMMON_H
#define COMMON_H

#include <cstddef>
#include <QColor>

namespace RcsXn {

constexpr size_t IO_COUNT = 2048;
constexpr size_t IO_OUT_MODULE_PIN_COUNT = 2;
constexpr size_t IO_IN_MODULE_PIN_COUNT = 8;
constexpr size_t IO_OUT_MODULES_COUNT = IO_COUNT / IO_OUT_MODULE_PIN_COUNT;
constexpr size_t IO_IN_MODULES_COUNT = IO_COUNT / IO_IN_MODULE_PIN_COUNT;
constexpr size_t SIGNAL_INIT_RESET_PERIOD = 200; // ms
constexpr size_t OUTPUT_ACTIVE_TIME = 500; // ms
constexpr size_t ACC_RESET_TIMER_PERIOD = 100; // ms

const QColor LOGC_ERROR = QColor(0xFF, 0xAA, 0xAA);
const QColor LOGC_WARN = QColor(0xFF, 0xFF, 0xAA);
const QColor LOGC_DONE = QColor(0xAA, 0xFF, 0xAA);
const QColor LOGC_GET = QColor(0xE0, 0xE0, 0xFF);
const QColor LOGC_PUT = QColor(0xE0, 0xFF, 0xE0);

enum class XnInState {
	unknown,
	off,
	on,
};

inline XnInState xnInState(bool state) {
	return (state) ? XnInState::on : XnInState::off;
}

} // namespace RcsXn

#endif // COMMON_H
