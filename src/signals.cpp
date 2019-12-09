#include "signals.h"
#include "lib/q-str-exception.h"

namespace RcsXn {

XnSignalTemplate::XnSignalTemplate() = default;

XnSignalTemplate::XnSignalTemplate(QSettings &s) { this->loadData(s); }

void XnSignalTemplate::loadData(QSettings &s) {
	// expects already beginned group
	for (const auto &k : s.childKeys()) {
		try {
			bool isNum = false;
			unsigned int scomCode = k.toUInt(&isNum);
			const QString output = s.value(k, "00").toString();
			if (isNum && isValidSignalOutputStr(output)) {
				this->outputs[scomCode] = output;
				this->outputsCount = static_cast<std::size_t>(output.length());
			}
		} catch (...) {
			// TODO
		}
	}
}

void XnSignalTemplate::saveData(QSettings &s) const {
	for (const std::pair<const unsigned int, QString> &output : this->outputs)
		s.setValue(QString::number(output.first), output.second);
}

XnSignal::XnSignal() = default;

XnSignal::XnSignal(QSettings &s, unsigned int hJOPaddr) : hJOPaddr(hJOPaddr) { this->loadData(s); }

void XnSignal::loadData(QSettings &s) {
	tmpl.loadData(s);
	this->startAddr = s.value("startAddr", this->hJOPaddr).toUInt();
	this->name = s.value("name", QString::number(this->hJOPaddr)).toString();
}

void XnSignal::saveData(QSettings &s) const {
	tmpl.saveData(s);
	if (this->startAddr != this->hJOPaddr)
		s.setValue("startAddr", this->startAddr);
	else
		s.remove("startAddr");
	if (this->name != "")
		s.setValue("name", this->name);
	else
		s.remove("name");
}

QString XnSignal::outputRange() const {
	if (this->tmpl.outputsCount == 0)
		return "-";
	if (this->tmpl.outputsCount == 1)
		return QString::number(this->tmpl.outputsCount);
	return QString::number(this->startAddr) + "-" +
	       QString::number(this->startAddr + this->tmpl.outputsCount);
}

bool isValidSignalOutputStr(const QString &str) {
	const QString ALLOWED_CHARS = "01+-N";
	for (const QChar& c : str)
		if (!ALLOWED_CHARS.contains(c))
			return false;
	return true;
}

SigStorage signalsFromFile(QSettings &s) {
	SigStorage result;

	for (const auto &g : s.childGroups()) {
		if (not g.startsWith("Signal"))
			continue;

		try {
			QStringList name = g.split('-');
			if (name.size() < 2)
				throw QStrException("Invalid signal: " + g);

			unsigned int hJOPoutput = name[1].toUInt(); // signal always at nibble 0

			s.beginGroup(g);
			result.emplace(hJOPoutput, XnSignal(s, hJOPoutput));
			s.endGroup();
		} catch (...) {
			throw QStrException("Invalid signal: " + g);
		}
	}

	return result;
}

SigTmplStorage signalTemplatesFromFile(QSettings &s) {
	SigTmplStorage result;

	for (const auto &g : s.childGroups()) {
		if (not g.startsWith("SigTemplate"))
			continue;

		try {
			QStringList name = g.split('-');
			if (name.size() < 2)
				throw QStrException("Invalid signal template: " + g);

			const QString sigName = name[1];

			s.beginGroup(g);
			result.emplace(sigName, XnSignalTemplate(s));
			s.endGroup();
		} catch (...) {
			throw QStrException("Invalid signal template: " + g);
		}
	}
	return result;
}

void signalsToFile(QSettings &s, const SigStorage &storage) {
	for (const auto &pair : storage) {
		QString group = "Signal-" + QString::number(pair.first);
		s.beginGroup(group);
		pair.second.saveData(s);
		s.endGroup();
	}
}

void signalTmplsToFile(QSettings &s, const SigTmplStorage &storage) {
	for (const auto &pair : storage) {
		s.beginGroup("SigTemplate-" + pair.first);
		pair.second.saveData(s);
		s.endGroup();
	}
}

} // namespace RcsXn
