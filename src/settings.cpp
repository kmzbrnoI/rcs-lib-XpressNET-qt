#include "settings.h"

void Settings::load(const QString& filename) {
	QSettings s(filename, QSettings::IniFormat);
	data.clear();

	for (const auto& g : s.childGroups()) {
		s.beginGroup(g);
		for (const auto& k : s.childKeys())
			data[g][k] = s.value(k, "");
		s.endGroup();
	}

	// Load default for non-loaded data
	for (const auto& gm : DEFAULTS)
		for (const std::pair<const QString, QVariant>& k : gm.second)
			if (data[gm.first].find(k.first) == data[gm.first].end())
				data[gm.first][k.first] = k.second;
}

void Settings::save(const QString& filename) {
	QSettings s(filename, QSettings::IniFormat);

	for (const auto& gm : data) {
		s.beginGroup(gm.first);
		for (const std::pair<const QString, QVariant>& k : gm.second)
			s.setValue(k.first, k.second);
		s.endGroup();
	}
}

std::map<QString, QVariant>& Settings::at(const QString& g) {
	return data[g];
}

std::map<QString, QVariant>& Settings::operator[] (const QString& g) {
	return at(g);
}

void Settings::cfgToUnsigned(std::map<QString, QVariant>& cfg, const QString& section,
                             unsigned& target) {
	if (cfg.find(section) != cfg.end())
		target = cfg[section].toInt();
	else
		cfg[section] = target;
}

void Settings::cfgToDouble(std::map<QString, QVariant>& cfg, const QString& section,
                           double& target) {
	if (cfg.find(section) != cfg.end())
		target = cfg[section].toDouble();
	else
		cfg[section] = target;
}

void Settings::cfgToQString(std::map<QString, QVariant>& cfg, const QString& section,
                            QString& target) {
	if (cfg.find(section) != cfg.end())
		target = cfg[section].toString();
	else
		cfg[section] = target;
}
