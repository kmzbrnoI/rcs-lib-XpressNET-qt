#ifndef SIGNALS_H
#define SIGNALS_H

#include <cstddef>
#include <map>
#include <QSettings>

namespace RcsXn {

class XnSignalTemplate {
	std::size_t outputsCount;
	std::map<unsigned int, uint16_t> outputs; // scom code -> outputs state

	XnSignalTemplate(QSettings &);
	void loadData(QSettings &);
	void saveData(QSettings &);
};

class XnSignal {
	unsigned int startAddr;
	XnSignalTemplate tmlp;
};

} // namespace RcsXn

#endif
