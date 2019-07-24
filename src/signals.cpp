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
		s.setValue(QString::number(output.first), QString::number(output.second, 2));
}

XnSignal::XnSignal() {}

XnSignal::XnSignal(QSettings &s) {
	this->loadData(s);
}

void XnSignal::loadData(QSettings &s) {
	tmpl.loadData(s);
	this->startAddr = s.value("startAddr", 0).toUInt();
}

void XnSignal::saveData(QSettings &s) const {
	tmpl.saveData(s);
	s.setValue("startAddr", this->startAddr);
}

} // namespace RcsXn
