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
	}},
};

class Settings {
public:
	Settings();
	Config data;

	void load(const QString &filename);
	void save(const QString &filename);

	std::map<QString, QVariant> &at(const QString &g);
	std::map<QString, QVariant> &operator[](const QString &g);

	static void cfgToUnsigned(std::map<QString, QVariant> &cfg, const QString &section,
	                          unsigned &target);
	static void cfgToDouble(std::map<QString, QVariant> &cfg, const QString &section,
	                        double &target);
	static void cfgToQString(std::map<QString, QVariant> &cfg, const QString &section,
	                         QString &target);

private:
	void loadDefaults();
};

#endif
