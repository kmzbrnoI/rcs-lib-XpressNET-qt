#ifndef RCSINPUTMODULE_H
#define RCSINPUTMODULE_H

#include "common.h"
#include <QSettings>
#include <QTimer>

namespace RcsXn {

struct RcsInputModule {
	unsigned addr;
	QString name;
	bool wantActive = false;
	bool realActive = false;
	std::array<unsigned, IO_IN_MODULE_PIN_COUNT> inputFallDelays; // [0.1s]: 10=1.0s, 5=0.5 s
	std::array<XnInState, IO_IN_MODULE_PIN_COUNT> state;
	QTimer inputFallTimers[IO_IN_MODULE_PIN_COUNT];

	RcsInputModule();

	void load(const QSettings&, unsigned addr);
	void save(QSettings&) const;
	static QString fallDelayToStr(unsigned fallDelay);
	bool allDefaults() const;
	QString defaultName() const;
	void stopAllFallTimers();
};

} // namespace RcsXn

#endif // RCSINPUTMODULE_H
