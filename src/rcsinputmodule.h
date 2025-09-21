#ifndef RCSINPUTMODULE_H
#define RCSINPUTMODULE_H

#include "common.h"
#include <QSettings>

namespace RcsXn {

struct RcsInputModule {
	unsigned addr;
	QString name;
	bool active = false;
	std::array<unsigned, IO_IN_MODULE_PIN_COUNT> inputFallDelays; // [0.1s]: 0-15 ~ 0.0-1.5 s
	std::array<bool, IO_IN_MODULE_PIN_COUNT> state;

	void load(const QSettings&, unsigned addr);
	void save(QSettings&) const;
	static QString fallDelayToStr(unsigned fallDelay);
	bool allDefaults() const;
	QString defaultName() const;
};

} // namespace RcsXn

#endif // RCSINPUTMODULE_H
