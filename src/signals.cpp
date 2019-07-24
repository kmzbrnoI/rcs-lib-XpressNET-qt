#include "signals.h"

namespace RcsXn {

XnSignalTemplate::XnSignalTemplate(QSettings &s) {
	this->loadData(s);
}

void XnSignalTemplate::loadData(QSettings &s) {
	// expects already beginned group
	for (const auto &k : s.childKeys()) {
		try {
			const QString bits = s.value(k, "00").toString();
			this->outputs[k.toInt()] = bits.toInt(nullptr, 2);
			this->outputsCount = bits.size();
		} catch (...) {
			// TODO
		}
	}
}

void XnSignalTemplate::saveData(QSettings &s) {
	for (const std::pair<unsigned int, uint16_t> &output : this->outputs)
		s.setValue(QString::number(output.first), QString::number(output.second, 2));
}

} // namespace RcsXn
