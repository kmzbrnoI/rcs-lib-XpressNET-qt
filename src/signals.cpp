#include "signals.h"

namespace RcsXn {

XnSignalTemplate::XnSignalTemplate() {}

XnSignalTemplate::XnSignalTemplate(QSettings &s) {
	this->loadData(s);
}

void XnSignalTemplate::loadData(QSettings &s) {
	// expects already beginned group
	for (const auto &k : s.childKeys()) {
		try {
			const QString bits = s.value(k, "00").toString();
			bool isNum = false;
			unsigned int scomCode = k.toInt(&isNum);
			if (isNum) {
				this->outputs[scomCode] = bits.toInt(nullptr, 2);
				this->outputsCount = bits.size();
			}
		} catch (...) {
			// TODO
		}
	}
}

void XnSignalTemplate::saveData(QSettings &s) const {
	for (const std::pair<unsigned int, uint16_t> &output : this->outputs)		
		s.setValue(QString::number(output.first),
				   QString("%1").arg(output.second, this->outputsCount, 2, QLatin1Char('0')));
}

XnSignal::XnSignal() {}

XnSignal::XnSignal(QSettings &s, unsigned int hJOPaddr) : hJOPaddr(hJOPaddr) {
	this->loadData(s);
}

void XnSignal::loadData(QSettings &s) {
	tmpl.loadData(s);
	this->startAddr = s.value("startAddr", this->hJOPaddr).toUInt();
}

void XnSignal::saveData(QSettings &s) const {
	tmpl.saveData(s);
	if (this->startAddr != this->hJOPaddr)
		s.setValue("startAddr", this->startAddr);
	else
		s.remove("startAddr");
}

} // namespace RcsXn
