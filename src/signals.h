#ifndef SIGNALS_H
#define SIGNALS_H

#include <cstddef>
#include <map>
#include <QSettings>

namespace RcsXn {

struct XnSignalTemplate {
	std::size_t outputsCount;
	std::map<unsigned int, uint16_t> outputs; // scom code -> outputs state

	XnSignalTemplate();
	XnSignalTemplate(QSettings &);
	void loadData(QSettings &);
	void saveData(QSettings &) const;
};

struct XnSignal {
	unsigned int startAddr;
	XnSignalTemplate tmpl;
	unsigned int hJOPaddr;

	XnSignal();
	XnSignal(QSettings &, unsigned int hJOPaddr);
	void loadData(QSettings &);
	void saveData(QSettings &) const;
};

} // namespace RcsXn

#endif
