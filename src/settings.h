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
		{"outIntervalMs", 50},
	}},
	{"global", {
		{"addrRange", "basic"},
		{"resetSignals", false},
		{"mockInputs", false},
		{"disableSetOutputOff", false},
	}},
	{"modules", {
		{"active-in", ""}, // unused, backward compatibility only
		{"active-out", "1-28,70-92"},
		{"binary", ""},
	}}
};

class Settings {
public:
	Settings();
	Config data;

	void load(QSettings& s, bool loadNonDefaults = true);
	void save(QSettings& s);

	std::map<QString, QVariant> &at(const QString &g);
	std::map<QString, QVariant> &operator[](const QString &g);

private:
	void loadDefaults();
};

#endif
