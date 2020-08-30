#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QString>
#include <map>

/* This file defines global settings of the application */

using Config = std::map<QString, std::map<QString, QVariant>>;

const Config DEFAULTS {
	{"XN", {
		{"port", "/dev/ttyUSB0"},
		{"baudrate", 19200},
		{"flowcontrol", 1},
		{"loglevel", 1},
		{"interface", "LI101"},
	}},
	{"global", {
		{"onlyOneActive", true},
		{"addrRange", "basic"},
		{"resetSignals", false},
		{"mockInputs", false},
		{"forbid00Output", true},
		{"disableSetOutputOff", false},
	}},
	{"modules", {
		{"active-in", "1-32,64-72"},
		{"active-out", "1-28,70-92"},
	}}
};

class Settings {
public:
	Settings();
	Config data;

	void load(const QString &filename, bool loadNonDefaults = true);
	void save(const QString &filename);

	std::map<QString, QVariant> &at(const QString &g);
	std::map<QString, QVariant> &operator[](const QString &g);

private:
	void loadDefaults();
};

#endif
