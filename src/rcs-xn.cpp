#include <QSettings>
#include <QTimer>
#include <algorithm>
#include <cstring>

#include "errors.h"
#include "rcs-xn.h"

namespace RcsXn {

AppThread main_thread;
RcsXn rx;

///////////////////////////////////////////////////////////////////////////////

RcsXn::RcsXn(QObject *parent) : QObject(parent), f_signal_edit(sigTemplates) {
	// XN events
	QObject::connect(&xn, SIGNAL(onError(QString)), this, SLOT(xnOnError(QString)));
	QObject::connect(&xn, SIGNAL(onLog(QString, Xn::LogLevel)), this,
	                 SLOT(xnOnLog(QString, Xn::LogLevel)));
	QObject::connect(&xn, SIGNAL(onConnect()), this, SLOT(xnOnConnect()));
	QObject::connect(&xn, SIGNAL(onDisconnect()), this, SLOT(xnOnDisconnect()));
	QObject::connect(&xn, SIGNAL(onTrkStatusChanged(Xn::TrkStatus)), this,
	                 SLOT(xnOnTrkStatusChanged(Xn::TrkStatus)));
	QObject::connect(
	    &xn, SIGNAL(onAccInputChanged(uint8_t, bool, bool, Xn::FeedbackType, Xn::AccInputsState)),
	    this, SLOT(xnOnAccInputChanged(uint8_t, bool, bool, Xn::FeedbackType, Xn::AccInputsState))
	);

	// No loading of configuration here (caller should call LoadConfig)

	this->guiInit();
	this->fillConnectionsCbs();

	log("Library loaded.", RcsXnLogLevel::llInfo);
	this->form.ui.tw_xn_log->resizeColumnToContents(0);
}

RcsXn::~RcsXn() {
	try {
		if (xn.connected())
			close();
		if (this->config_filename != "")
			this->saveConfig(this->config_filename);
	} catch (...) {
		// No exceptions in destructor!
	}
}

void RcsXn::log(const QString &msg, RcsXnLogLevel loglevel) {
	if (loglevel > this->loglevel)
		return;

	// UI
	if (form.ui.tw_xn_log->topLevelItemCount() > 300)
		form.ui.tw_xn_log->clear();

	auto *item = new QTreeWidgetItem(form.ui.tw_xn_log);
	item->setText(0, QTime::currentTime().toString("hh:mm:ss"));

	if (msg.startsWith("GET:"))
		for (int i = 0; i < 3; i++)
			item->setBackground(i, LOGC_GET);
	if (msg.startsWith("PUT:"))
		for (int i = 0; i < 3; i++)
			item->setBackground(i, LOGC_PUT);

	if (loglevel == RcsXnLogLevel::llNo)
		item->setText(1, "Nic");
	else if (loglevel == RcsXnLogLevel::llError) {
		item->setText(1, "Chyba");
		for (int i = 0; i < 3; i++)
			item->setBackground(i, LOGC_ERROR);
	} else if (loglevel == RcsXnLogLevel::llWarning) {
		item->setText(1, "Varování");
		for (int i = 0; i < 3; i++)
			item->setBackground(i, LOGC_WARN);
	} else if (loglevel == RcsXnLogLevel::llInfo)
		item->setText(1, "Info");
	else if (loglevel == RcsXnLogLevel::llCommands)
		item->setText(1, "Příkaz");
	else if (loglevel == RcsXnLogLevel::llRawCommands)
		item->setText(1, "Data");
	else if (loglevel == RcsXnLogLevel::llDebug)
		item->setText(1, "Debug");

	QLabel *label = new QLabel();
	label->setWordWrap(true);
	label->setText(msg);

	form.ui.tw_xn_log->insertTopLevelItem(0, item);
	form.ui.tw_xn_log->setItemWidget(item, 2, label);

	// event
	this->events.call(this->events.onLog, static_cast<int>(loglevel), msg);
}

void RcsXn::setLogLevel(RcsXnLogLevel loglevel) {
	this->loglevel = loglevel;
	xn.loglevel = static_cast<Xn::LogLevel>(loglevel);
	s["XN"]["loglevel"] = static_cast<int>(loglevel);
}

void RcsXn::error(const QString &message, uint16_t code, unsigned int module) {
	this->events.call(this->events.onError, code, module, message);
}

void RcsXn::error(const QString &message, uint16_t code) { this->error(message, code, 0); }
void RcsXn::error(const QString &message) { this->error(message, RCS_GENERAL_EXCEPTION, 0); }

int RcsXn::openDevice(const QString &device, bool persist) {
	if (xn.connected())
		return RCS_ALREADY_OPENNED;

	this->resetIOState();
	events.call(rx.events.beforeOpen);
	this->guiOnOpen();

	try {
		xn.connect(device, s["XN"]["baudrate"].toInt(),
		           static_cast<QSerialPort::FlowControl>(s["XN"]["flowcontrol"].toInt()),
		           interface(s["XN"]["interface"].toString()));
	} catch (const Xn::QStrException &e) {
		error("XN connect error while opening serial port '" +
		      s["XN"]["port"].toString() + "':" + e, RCS_CANNOT_OPEN_PORT);
		events.call(rx.events.afterClose);
		this->guiOnClose();
		return RCS_CANNOT_OPEN_PORT;
	}

	if (persist)
		s["XN"]["port"] = device;

	return 0;
}

int RcsXn::close() {
	events.call(rx.events.beforeClose);

	if (!xn.connected())
		return RCS_NOT_OPENED;

	if (this->started > RcsStartState::stopped)
		return RCS_SCANNING_NOT_FINISHED;

	this->opening = false;
	try {
		xn.disconnect();
	} catch (const Xn::QStrException &e) {
		error("XN disconnect error while closing serial port:" + e);
	}

	this->resetIOState();
	return 0;
}

int RcsXn::start() {
	if (this->started == RcsStartState::started)
		return RCS_ALREADY_STARTED;
	if (this->started == RcsStartState::scanning)
		return RCS_SCANNING_NOT_FINISHED;
	if (!xn.connected())
		return RCS_NOT_OPENED;

	log("Spouštím komunikaci...", RcsXnLogLevel::llInfo);
	events.call(rx.events.beforeStart);
	started = RcsStartState::scanning;
	events.call(rx.events.afterStart);
	log("Komunikace běží.", RcsXnLogLevel::llInfo);
	this->first_scan();
	return 0;
}

int RcsXn::stop() {
	if (rx.started == RcsStartState::stopped)
		return RCS_NOT_STARTED;

	log("Zastavuji komunikaci...", RcsXnLogLevel::llInfo);
	events.call(rx.events.beforeStop);
	this->started = RcsStartState::stopped;
	events.call(rx.events.afterStop);
	log("Komunikace zastavena", RcsXnLogLevel::llInfo);
	return 0;
}

void RcsXn::loadConfig(const QString &filename) {
	s.load(filename, false); // do not load & store nonDefaults
	this->loglevel = static_cast<RcsXnLogLevel>(s["XN"]["loglevel"].toInt());
	this->xn.loglevel = static_cast<Xn::LogLevel>(s["XN"]["loglevel"].toInt());

	this->gui_config_changing = true;
	try {
		form.ui.cb_loglevel->setCurrentIndex(static_cast<int>(this->loglevel));
		form.ui.chb_only_one_active->setChecked(s["global"]["onlyOneActive"].toBool());
		form.ui.chb_reset_signals->setChecked(s["global"]["resetSignals"].toBool());
		if (s["global"]["addrRange"].toString() == "basic")
			form.ui.cb_addr_range->setCurrentIndex(0);
		else if (s["global"]["addrRange"].toString() == "lenz")
			form.ui.cb_addr_range->setCurrentIndex(1);
		else {
			s["global"]["addrRange"] = "basic";
			form.ui.cb_addr_range->setCurrentIndex(0);
		}
		this->fillConnectionsCbs();

		try {
			QSettings s(filename, QSettings::IniFormat);
			s.setIniCodec("UTF-8");
			this->sig = signalsFromFile(s);
			this->sigTemplates = signalTemplatesFromFile(s);
		} catch (const QStrException &e) {
			this->log("Nepodařilo se načíst návěstidla: " + e.str(), RcsXnLogLevel::llError);
			throw;
		}
		this->fillSignals();

		try {
			this->loadActiveIO(s["modules"]["active-in"].toString(), s["modules"]["active-out"].toString(),
							   false);
		} catch (const QStrException &e) {
			this->log("Nepodařilo se načíst aktivní vstupy a výstupy: " + e.str(), RcsXnLogLevel::llError);
			throw;
		}
		this->fillActiveIO();
		this->gui_config_changing = false;
	} catch (...) {
		this->fillSignals();
		this->fillActiveIO();

		this->gui_config_changing = false;
		throw;
	}
}

void RcsXn::first_scan() {
	log("Skenuji stav aktivních vstupů...", RcsXnLogLevel::llInfo);
	this->scanNextGroup(0);
}

void RcsXn::saveConfig(const QString &filename) {
	s["modules"]["active-in"] = getActiveStr(this->active_in, ",");
	s["modules"]["active-out"] = getActiveStr(this->active_out, ",");

	s.save(filename);
	this->saveSignals(filename);
}

void RcsXn::loadActiveIO(const QString &inputs, const QString &outputs, bool except) {
	this->parseActiveModules(inputs, this->active_in, except);
	this->parseActiveModules(outputs, this->active_out, except);

	this->modules_count = this->in_count = this->out_count = 0;
	for (size_t i = 0; i < IO_IN_MODULES_COUNT; i++)
		if (this->active_in[i])
			this->in_count++;
	for (size_t i = 0; i < IO_OUT_MODULES_COUNT; i++)
		if (this->active_out[i])
			this->out_count++;
	for (size_t i = 0; i < std::max(IO_IN_MODULES_COUNT, IO_OUT_MODULES_COUNT); i++)
		if ((i < IO_IN_MODULES_COUNT && this->active_in[i]) ||
				(i < IO_OUT_MODULES_COUNT && this->active_out[i]))
			this->modules_count++;

	if ((s["global"]["addrRange"].toString() == "lenz") && (this->active_out[0]))
		throw EInvalidRange("Adresa 0 není validní adresou systému Lenz!");
}

void RcsXn::initModuleScanned(uint8_t group, bool nibble) {
	static int nibbles_scanned = 0;
	nibbles_scanned |= static_cast<int>(nibble)+1; // fill 2 LSBs

	if (nibbles_scanned != 3) // not both nibbles scanned -> wait for other nibble
		return;

	nibbles_scanned = 0;
	this->scanNextGroup(group);
}

void RcsXn::scanNextGroup(uint8_t previousGroup) {
	unsigned int nextGroup = previousGroup+1;

	while ((nextGroup < IO_IN_MODULES_COUNT) && (!this->active_in[nextGroup]))
		nextGroup += 1;

	if (nextGroup >= IO_IN_MODULES_COUNT) {
		this->initScanningDone();
		return;
	}

	this->scan_group = nextGroup;

	// Scan both nibbles
	xn.accInfoRequest(
	    this->scan_group, false,
	    std::make_unique<Xn::Cb>([this](void *s, void *d) { xnOnInitScanningError(s, d); })
	);
	xn.accInfoRequest(
	    this->scan_group, true,
	    std::make_unique<Xn::Cb>([this](void *s, void *d) { xnOnInitScanningError(s, d); })
	);
}

void RcsXn::xnOnInitScanningError(void *, void *) {
	error("Module scanning: no response!", RCS_NOT_OPENED);
	this->stop();
}

void RcsXn::initScanningDone() {
	log("Stav vstupů naskenován.", RcsXnLogLevel::llInfo);
	this->started = RcsStartState::started;
	events.call(events.onScanned);

	if (this->s["global"]["resetSignals"].toBool())
		this->resetSignals();
}

void RcsXn::xnSetOutputError(void *sender, void *data) {
	(void)sender;
	// TODO: mark module as failed?
	unsigned int module = reinterpret_cast<uintptr_t>(data);
	error("Command Station did not respond to SetOutput command!", RCS_MODULE_NOT_ANSWERED_CMD,
	      module);
}

int RcsXn::setPlainOutput(unsigned int portAddr, int state) {
	unsigned int module = portAddr / IO_OUT_MODULE_PIN_COUNT;
	unsigned int port = portAddr % IO_OUT_MODULE_PIN_COUNT;

	if ((state > 0) && (s["global"]["onlyOneActive"].toBool())) {
		unsigned int secondPort = (module<<1) + !(port&1); // 0-2047
		outputs[secondPort] = false;
	}
	outputs[portAddr] = static_cast<bool>(state);

	unsigned int realPortAddr = portAddr;
	if (s["global"]["addrRange"].toString() == "lenz") {
		if (module == 0) {
			log("Invalid acc port (using Lenz addresses): " + QString::number(portAddr),
				RcsXnLogLevel::llWarning);
			return RCS_PORT_INVALID_NUMBER;
		}
		realPortAddr -= IO_OUT_MODULE_PIN_COUNT;
	}

	xn.accOpRequest(
		static_cast<uint16_t>(realPortAddr), static_cast<bool>(state), nullptr,
		std::make_unique<Xn::Cb>([this](void *s, void *d) { this->xnSetOutputError(s, d); },
								 reinterpret_cast<void *>(module))
	);

	events.call(events.onOutputChanged, module); // TODO: move to ok callback?
	return 0;
}

template <std::size_t ArraySize>
void RcsXn::parseActiveModules(const QString &active, std::array<bool, ArraySize> &result,
                               bool except) {
	std::fill(result.begin(), result.end(), false);

	const QStringList ranges = active.split(',', QString::SkipEmptyParts);
	for (const QString &range : ranges) {
		const QStringList bounds = range.split('-');
		bool okl, okr = false;
		if (bounds.size() == 1) {
			unsigned int addr = bounds[0].toUInt(&okl);
			if ((okl) && (addr < result.size())) {
				result[addr] = true;
			} else {
				if (except)
					throw EInvalidRange("Invalid range: " + bounds[0]);
				log("Invalid range: " + bounds[0], RcsXnLogLevel::llWarning);
			}
		} else if (bounds.size() == 2) {
			unsigned int left = bounds[0].toUInt(&okl);
			unsigned int right = bounds[1].toUInt(&okr);
			if ((okl) & (okr) && (left < result.size()) && (right < result.size())) {
				for (size_t i = left; i <= right; i++)
					result[i] = true;
			} else {
				if (except)
					throw EInvalidRange("Invalid range: " + range);
				log("Invalid range: " + range, RcsXnLogLevel::llWarning);
			}
		} else {
			if (except)
				throw EInvalidRange("Invalid range: " + range);
			log("Invalid range: " + range, RcsXnLogLevel::llWarning);
		}
	}
}

template <std::size_t ArraySize>
QString RcsXn::getActiveStr(const std::array<bool, ArraySize> &source, const QString &separator) {
	QString output;
	for (size_t start = 0; start < source.size(); ++start) {
		if (source[start]) {
			size_t end = start;
			while (source[end] && end < source.size())
				++end;
			if (end == start+1)
				output += QString::number(start)+separator;
			else
				output += QString::number(start)+"-"+QString::number(end-1)+separator;
			start = end;
		}
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// Xn events

void RcsXn::xnOnError(QString error) {
	// Xn error is considered fatal -> close device
	this->error(error, RCS_FT_EXCEPTION);

	if (this->started != RcsStartState::stopped)
		this->stop();
	if (xn.connected())
		this->close();
}

void RcsXn::xnOnLog(QString message, Xn::LogLevel loglevel) {
	this->log(message, static_cast<RcsXnLogLevel>(loglevel));
}

void RcsXn::xnOnConnect() {
	this->opening = true;

	try {
		xn.getLIVersion(
		    [this](void *s, unsigned hw, unsigned sw) { xnGotLIVersion(s, hw, sw); },
		    std::make_unique<Xn::Cb>([this](void *s, void *d) { xnOnLIVersionError(s, d); })
		);
	} catch (const Xn::QStrException& e) {
		error("Get LI Version: " + e.str(), RCS_NOT_OPENED);
		this->close();
	}
}

void RcsXn::xnOnDisconnect() {
	this->events.call(this->events.afterClose);
	this->guiOnClose();
}

void RcsXn::xnOnTrkStatusChanged(Xn::TrkStatus s) {
	form.ui.b_dcc_on->setEnabled((s == Xn::TrkStatus::Off));
	form.ui.b_dcc_off->setEnabled((s == Xn::TrkStatus::On));

	if (s == Xn::TrkStatus::On) {
		form.ui.l_dcc_state->setText("ON");
		widgetSetColor(*form.ui.l_dcc_state, Qt::green);
	} else if (s == Xn::TrkStatus::Off) {
		form.ui.l_dcc_state->setText("OFF");
		widgetSetColor(*form.ui.l_dcc_state, Qt::red);
	} else if (s == Xn::TrkStatus::Programming) {
		form.ui.l_dcc_state->setText("PROGRAM");
		widgetSetColor(*form.ui.l_dcc_state, Qt::yellow);
	} else {
		form.ui.l_dcc_state->setText("???");
		widgetSetColor(*form.ui.l_dcc_state, Qt::black);
	}

	if (this->opening) {
		if (s != Xn::TrkStatus::On) {
			try {
				xn.setTrkStatus(
					Xn::TrkStatus::On, nullptr,
					std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_onDccOpenError(s, d); })
				);
			} catch (const Xn::QStrException& e) {
				log("SetTrkStatus error: " + e.str(), RcsXnLogLevel::llError);
				this->close();
			}
		} else {
			this->opening = false;
			this->events.call(this->events.afterOpen);
		}
	}
}

void RcsXn::xnOnAccInputChanged(uint8_t groupAddr, bool nibble, bool error,
                                Xn::FeedbackType inputType, Xn::AccInputsState state) {
	(void)error; // ignoring errors reported by decoders
	(void)inputType; // ignoring input type reported by decoder

	unsigned int port = 8*groupAddr + 4*nibble;

	this->inputs[port+0] = state.sep.i0;
	this->inputs[port+1] = state.sep.i1;
	this->inputs[port+2] = state.sep.i2;
	this->inputs[port+3] = state.sep.i3;

	if ((this->started == RcsStartState::scanning) && (groupAddr == this->scan_group)) {
		this->initModuleScanned(groupAddr, nibble);
		return;
	}

	if (!this->active_in[groupAddr]) {
		if (!form.ui.chb_scan_inputs->isChecked())
			return; // no scanning -> ignore change
		this->active_in[groupAddr] = true;
		this->modules_count++;
		this->in_count++;
		try { this->fillActiveIO(); } catch (...) {}
	}

	events.call(events.onInputChanged, groupAddr);
}

void RcsXn::xnOnLIVersionError(void *, void *) {
	error("Get LI Version: no response!", RCS_NOT_OPENED);
	this->close();
}

void RcsXn::xnOnCSStatusError(void *, void *) {
	error("Get CS Status: no response!", RCS_NOT_OPENED);
	this->close();
}

void RcsXn::xnGotLIVersion(void *, unsigned hw, unsigned sw) {
	log("Got LI version. HW: " + QString::number(hw) + ", SW: " + QString::number(sw),
	    RcsXnLogLevel::llInfo);
	this->li_ver_hw = hw;
	this->li_ver_sw = sw;

	try {
		xn.getCommandStationStatus(
		    nullptr,
		    std::make_unique<Xn::Cb>([this](void *s, void *d) { xnOnCSStatusError(s, d); })
		);
	} catch (const Xn::QStrException& e) {
		error("Get CS Status: " + e.str(), RCS_NOT_OPENED);
		this->close();
	}
}

///////////////////////////////////////////////////////////////////////////////
// Signals

void RcsXn::saveSignals(const QString &filename) {
	QSettings s(filename, QSettings::IniFormat);
	s.setIniCodec("UTF-8");

	// delete sections
	for (const auto &g : s.childGroups()) {
		if (g.startsWith("Signal") || g.startsWith("SigTemplate")) {
			s.beginGroup(g);
			s.remove("");
			s.endGroup();
		}
	}

	signalsToFile(s, this->sig);
	signalTmplsToFile(s, this->sigTemplates);
}

bool RcsXn::isSignal(unsigned int portAddr) const {
	return ((!(portAddr&1)) && (this->sig.find(portAddr >> 1) != this->sig.end()));
}

void RcsXn::setSignal(unsigned int portAddr, unsigned int code) {
	XnSignal &sig = this->sig.at(portAddr/IO_OUT_MODULE_PIN_COUNT);
	sig.currentCode = code;
	rx.events.call(rx.events.onOutputChanged, sig.hJOPaddr);

	if (sig.tmpl.outputs.find(code) == sig.tmpl.outputs.end())
		return; // no ports assignment for this signal code
	QString outputs = sig.tmpl.outputs.at(code);

	for (size_t i = 0; i < std::min(sig.tmpl.outputsCount, static_cast<std::size_t>(outputs.length())); i++) {
		QChar state = outputs[static_cast<int>(i)];
		if (state == 'N')
			continue;

		const unsigned int module = sig.startAddr+i;

		if (state == '+')
			this->setPlainOutput(2*module, true);
		else if (state == '-')
			this->setPlainOutput(2*module + 1, true);
		else if (state == '0') {
			this->setPlainOutput(2*module, false);
			this->setPlainOutput(2*module + 1, false);
		} else if (state == '1') {
			this->setPlainOutput(2*module, true);
			this->setPlainOutput(2*module + 1, true);
		}
	}
}

void RcsXn::resetSignals() {
	static QTimer timer;
	static auto it = this->sig.begin();

	if (!timer.isActive()) {
		log("Nastavuji návěstidla na stůj...", RcsXnLogLevel::llInfo);
		it = this->sig.begin();
		QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(resetSignals()));
		timer.setInterval(200);
		timer.start();
	} else {
		if (it == this->sig.end() || !this->xn.connected()) {
			timer.stop();
			log("Návěstidla nastavena na stůj.", RcsXnLogLevel::llInfo);
			return;
		}
	}

	this->setSignal(it->second.startAddr * IO_OUT_MODULE_PIN_COUNT, 0);
	++it;
}

void RcsXn::resetIOState() {
	std::fill(this->outputs.begin(), this->outputs.end(), false);
	std::fill(this->inputs.begin(), this->inputs.end(), false);
	for (auto &signal : this->sig)
		signal.second.currentCode = 0;
}

///////////////////////////////////////////////////////////////////////////////

Xn::LIType RcsXn::interface(QString name) {
	if (name == "LI101")
		return Xn::LIType::LI101;
	if (name == "uLI")
		return Xn::LIType::uLI;
	return Xn::LIType::LI100;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace RcsXn
