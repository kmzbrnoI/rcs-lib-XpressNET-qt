#ifndef RCSINPUTMODULE_H
#define RCSINPUTMODULE_H

#include <QString>
#include <array>

class RcsInputModule {
public:
	QString name;
	bool active = false;
	std::array<unsigned, IO_IN_MODULE_PIN_COUNT> inputFallDelays; // [0.1s]: 0-15 ~ 0.0-1.5 s
	std::array<bool, IO_IN_MODULE_PIN_COUNT> state;

	void load(const QSettings&, const QString& section);
	void save(QSettings&, const QString& section);
	static QString fallDelayToStr(unsigned fallDelay);
};

#endif // RCSINPUTMODULE_H
