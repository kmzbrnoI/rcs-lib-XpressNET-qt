#ifndef SIGNALS_H
#define SIGNALS_H

#include <QSettings>
#include <QString>
#include <cstddef>
#include <map>

namespace RcsXn {

struct XnSignalTemplate {
	std::size_t outputsCount;
	std::map<unsigned int, QString> outputs; // scom code -> outputs state

	XnSignalTemplate();
	XnSignalTemplate(QSettings &);
	void loadData(QSettings &);
	void saveData(QSettings &) const;
};

struct XnSignal {
	QString name;
	unsigned int startAddr; // 0-1023
	XnSignalTemplate tmpl;
	unsigned int hJOPaddr;
	unsigned int currentCode;

	XnSignal();
	XnSignal(QSettings &, unsigned int hJOPaddr);
	void loadData(QSettings &);
	void saveData(QSettings &) const;
	QString outputRange() const;
};

const std::array<QString, 16> XnSignalCodes {
    "Stůj/posun zakázán",
    "Volno",
    "Výstraha",
    "Očekávejte 40 km/h",
    "40 km/h a volno",
    "Svítí vše (rezerva)",
    "40 km/h a výstraha",
    "40 km/h a očekávejte 40 km/h",
    "Přivolávací návěst",
    "Dovolen zajištěný posun",
    "Dovolen nezajištěný posun",
    "Opakování návěsti volno",
    "Opakování návěsti výstraha",
    "Návěstidlo zhaslé",
    "Opak. návěsti očekávejte 40 km/h",
    "Opak. návěsti výstraha a 40 km/h",
};

using SigTmplStorage = std::map<QString, XnSignalTemplate>;
using SigStorage = std::map<unsigned int, XnSignal>; // hJOP output -> signal mapping

bool isValidSignalOutputStr(const QString &str);
SigStorage signalsFromFile(QSettings &);
SigTmplStorage signalTemplatesFromFile(QSettings &);
void signalsToFile(QSettings &s, const SigStorage &storage);
void signalTmplsToFile(QSettings &s, const SigTmplStorage &storage);

} // namespace RcsXn

#endif
