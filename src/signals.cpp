#include "signals.h"

namespace RcsXn {

XnSignalTemplate::XnSignalTemplate() = default;

XnSignalTemplate::XnSignalTemplate(QSettings &s) { this->loadData(s); }

void XnSignalTemplate::loadData(QSettings &s) {
	// expects already beginned group
	for (const auto &k : s.childKeys()) {
		try {
			const QString bits = s.value(k, "00").toString();
			bool isNum = false;
			unsigned int scomCode = k.toUInt(&isNum);
			if (isNum) {
				this->outputs[scomCode] = bits.toUShort(nullptr, 2);
				this->outputsCount = static_cast<std::size_t>(bits.size());
			}
		} catch (...) {
			// TODO
		}
	}
}

void XnSignalTemplate::saveData(QSettings &s) const {
	for (const std::pair<const unsigned int, uint16_t> &output : this->outputs)
		s.setValue(QString::number(output.first),
				   QString("%1").arg(output.second, static_cast<int>(this->outputsCount), 2, QLatin1Char('0')));
}

XnSignal::XnSignal() = default;

XnSignal::XnSignal(QSettings &s, unsigned int hJOPaddr) : hJOPaddr(hJOPaddr) { this->loadData(s); }

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
