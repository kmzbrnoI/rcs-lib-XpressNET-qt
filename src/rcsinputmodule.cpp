#include "rcs-xn.h"

namespace RcsXn {

QString RcsInputModule::fallDelayToStr(unsigned fallDelay) {
	return QString::number(fallDelay/10) + "." + QString::number(fallDelay%10);
}

void RcsInputModule::load(const QSettings& s, unsigned addr) {
	this->addr = addr;
	this->name = s.value("name", this->defaultName()).toString();
	this->wantActive = s.value("active", false).toBool();
	for (unsigned i = 0; i < IO_IN_MODULE_PIN_COUNT; i++) {
		const QString fallDelay = s.value("fallDelay"+QString::number(i+1), "0.0").toString();
		if (fallDelay.length() >= 3)
			this->inputFallDelays[i] = (QString(fallDelay[0]) + QString(fallDelay[2])).toInt();
		else
			this->inputFallDelays[i] = 0;
	}
}

void RcsInputModule::save(QSettings& s) const {
	if (this->allDefaults())
		return; // do not save default module; if module is not default, save everything

	s.setValue("name", this->name);
	s.setValue("active", this->wantActive);
	for (unsigned i = 0; i < IO_IN_MODULE_PIN_COUNT; i++) {
		const QString key = "fallDelay"+QString::number(i+1);
		if (this->inputFallDelays[i] == 0)
			s.remove(key);
		else
			s.setValue(key, RcsInputModule::fallDelayToStr(this->inputFallDelays[i]));
	}
}

QString RcsInputModule::defaultName() const {
	return "Module "+QString::number(addr);
}

bool RcsInputModule::allDefaults() const {
	if (this->name != this->defaultName())
		return false;
	if (this->wantActive)
		return false;
	for (unsigned i = 0; i < IO_IN_MODULE_PIN_COUNT; i++)
		if (this->inputFallDelays[i] != 0)
			return false;

	return true;
}

void RcsXn::loadInputModules(QSettings &s) {
	try {
		for (unsigned i = 0; i < IO_IN_MODULES_COUNT; i++) {
			s.beginGroup("InModule-"+QString::number(i));
			this->modules_in[i].load(s, i);
			s.endGroup();
		}
	} catch (const QStrException &e) {
		this->log("Nepodařilo se načíst vstupní moduly: " + e.str(), RcsXnLogLevel::llError);
		throw;
	}
	this->twFillInputModules();
	this->refreshActiveIOCounts();
}

void RcsXn::saveInputModules(QSettings &s) const {
	s.beginGroup("modules");
	s.remove("active-in");
	s.endGroup();

	for (unsigned i = 0; i < IO_IN_MODULES_COUNT; i++) {
		s.beginGroup("InModule-"+QString::number(i));
		this->modules_in[i].save(s);
		s.endGroup();
	}
}

} // namespace RcsXn
