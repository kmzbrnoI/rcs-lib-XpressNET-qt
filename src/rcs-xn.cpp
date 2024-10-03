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
	QObject::connect(&m_acc_reset_timer, SIGNAL(timeout()), this, SLOT(m_acc_reset_timer_tick()));
	m_acc_reset_timer.setInterval(ACC_RESET_TIMER_PERIOD);
	m_acc_reset_timer.start();

	QObject::connect(&m_resetSignalsTimer, SIGNAL(timeout()), this, SLOT(resetNextSignal()));
	m_resetSignalsTimer.setInterval(SIGNAL_INIT_RESET_PERIOD);

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
	constexpr size_t COLUMN_COUNT = 3;

	if (loglevel > this->loglevel)
		return;

	// UI
	if (form.ui.tw_xn_log->topLevelItemCount() > 300)
		form.ui.tw_xn_log->clear();

	auto *item = new QTreeWidgetItem(form.ui.tw_xn_log);
	item->setText(0, QTime::currentTime().toString("hh:mm:ss,zzz"));

	if (msg.startsWith("GET:"))
		for (size_t i = 0; i < COLUMN_COUNT; i++)
			item->setBackground(i, LOGC_GET);
	if (msg.startsWith("PUT:"))
		for (size_t i = 0; i < COLUMN_COUNT; i++)
			item->setBackground(i, LOGC_PUT);

	if (loglevel == RcsXnLogLevel::llNo)
		item->setText(1, "Nic");
	else if (loglevel == RcsXnLogLevel::llError) {
		item->setText(1, "Chyba");
		for (size_t i = 0; i < COLUMN_COUNT; i++)
			item->setBackground(i, LOGC_ERROR);
	} else if (loglevel == RcsXnLogLevel::llWarning) {
		item->setText(1, "Varování");
		for (size_t i = 0; i < COLUMN_COUNT; i++)
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

	form.ui.tw_xn_log->addTopLevelItem(item);
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
		const QString errMsg = "XN connect error while opening serial port '" +
			s["XN"]["port"].toString() + "': " + e;
		error(errMsg, RCS_CANNOT_OPEN_PORT);
		log(errMsg, RcsXnLogLevel::llError);
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
	this->resetIOState();
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
	std::fill(this->real_active_in.begin(), this->real_active_in.end(), false);
	this->resetIOState();
	events.call(rx.events.afterStop);
	log("Komunikace zastavena", RcsXnLogLevel::llInfo);
	return 0;
}

void RcsXn::loadConfig(const QString &filename) {
	s.load(filename, false); // do not load & store nonDefaults

	bool ok;
	this->loglevel = static_cast<RcsXnLogLevel>(s["XN"]["loglevel"].toInt(&ok));
	this->xn.loglevel = static_cast<Xn::LogLevel>(s["XN"]["loglevel"].toInt());
	if (!ok)
		throw QStrException("logLevel invalid type!");

	Xn::XNConfig xnconfig;
	xnconfig.outInterval = s["XN"]["outIntervalMs"].toUInt(&ok);
	if (!ok)
		throw QStrException("outIntervalMs invalid type!");
	this->xn.setConfig(xnconfig);

	this->gui_config_changing = true;
	try {
		form.ui.cb_loglevel->setCurrentIndex(static_cast<int>(this->loglevel));
		form.ui.chb_reset_signals->setChecked(s["global"]["resetSignals"].toBool());
		form.ui.chb_disable_set_output_off->setChecked(s["global"]["disableSetOutputOff"].toBool());
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
			this->loadActiveIO(s["modules"]["active-in"].toString(),
			                   s["modules"]["active-out"].toString(), false);
		} catch (const QStrException &e) {
			this->log("Nepodařilo se načíst aktivní vstupy a výstupy: " + e.str(),
			          RcsXnLogLevel::llError);
			throw;
		}
		this->fillActiveIO();

		try {
			this->parseModules(s["modules"]["binary"].toString(), this->binary, false);
		} catch (const QStrException &e) {
			this->log("Nepodařilo se načíst binární výstupy: " + e.str(),
			          RcsXnLogLevel::llError);
			throw;
		}
		form.ui.te_binary_outputs->setText(getActiveStr(this->binary, ",\n"));

		this->gui_config_changing = false;
	} catch (...) {
		this->fillSignals();
		this->fillActiveIO();
		form.ui.te_binary_outputs->setText(getActiveStr(this->binary, ",\n"));

		this->gui_config_changing = false;
		throw;
	}
}

void RcsXn::first_scan() {
	log("Skenuji stav aktivních vstupů...", RcsXnLogLevel::llInfo);
	std::fill(this->real_active_in.begin(), this->real_active_in.end(), false);
	this->scanNextGroup(-1);
}

void RcsXn::saveConfig(const QString &filename) {
	s["modules"]["active-in"] = getActiveStr(this->user_active_in, ",");
	s["modules"]["active-out"] = getActiveStr(this->user_active_out, ",");
	s["modules"]["binary"] = getActiveStr(this->binary, ",");

	s.save(filename);
	this->saveSignals(filename);
}

void RcsXn::loadActiveIO(const QString &inputs, const QString &outputs, bool except) {
	this->parseModules(inputs, this->user_active_in, except);
	this->parseModules(outputs, this->user_active_out, except);

	this->modules_count = this->in_count = this->out_count = 0;
	for (size_t i = 0; i < IO_IN_MODULES_COUNT; i++)
		if (this->user_active_in[i])
			this->in_count++;
	for (size_t i = 0; i < IO_OUT_MODULES_COUNT; i++)
		if (this->user_active_out[i])
			this->out_count++;
	for (size_t i = 0; i < std::max(IO_IN_MODULES_COUNT, IO_OUT_MODULES_COUNT); i++)
		if ((i < IO_IN_MODULES_COUNT && this->user_active_in[i]) ||
		    (i < IO_OUT_MODULES_COUNT && this->user_active_out[i]))
			this->modules_count++;

	if ((s["global"]["addrRange"].toString() == "lenz") && (this->user_active_out[0]))
		throw EInvalidRange("Adresa výstupu 0 není validní adresou systému Lenz!");
	if ((s["global"]["addrRange"].toString() == "lenz") && (this->user_active_in[0]))
		throw EInvalidRange("Adresa vstupního modulu 0 není validní adresou systému Lenz!");
}

void RcsXn::initModuleScanned(uint8_t group, bool nibble) {
	static int nibbles_scanned = 0;
	int received_nibble = static_cast<int>(nibble)+1;

	if (this->started != RcsStartState::scanning)
		return;

	if (nibbles_scanned == received_nibble) {
		// LZV100 does this: it responds 2 times with same nibble when module not connected
		// -> go to next message directly
		uint8_t old_scan_group = this->scan_group;
		log("Module scanning: invalid response!", RcsXnLogLevel::llError);
		xn.histClear();
		if (this->scan_group == old_scan_group && this->started != RcsStartState::started) {
			// Buffer clear did not automatically call scanNextGroup -> call it manually
			nibbles_scanned = 0;
			this->scanNextGroup(this->scan_group);
		}
		return;
	}

	nibbles_scanned |= received_nibble; // fill 2 LSBs

	if (nibbles_scanned == 1 && this->xn.liType() == Xn::LIType::LIUSBEth) {
		// LI-USB-Eth has problems with multiple commands -> send serially
		xn.accInfoRequest(
			inBusModuleAddr(this->scan_group), true,
			std::make_unique<Xn::Cb>([this](void *s, void *d) { xnOnInitScanningError(s, d); },
									 reinterpret_cast<void *>(true))
		);
	}

	if (nibbles_scanned != 3) // not both nibbles scanned -> wait for other nibble
		return;

	nibbles_scanned = 0;
	this->scanNextGroup(group);
}

void RcsXn::scanNextGroup(int previousGroup) {
	unsigned int nextGroup = previousGroup+1;

	while ((nextGroup < IO_IN_MODULES_COUNT) && (!this->user_active_in[nextGroup]))
		nextGroup += 1;

	if (nextGroup >= IO_IN_MODULES_COUNT) {
		this->initScanningDone();
		return;
	}

	this->scan_group = nextGroup;

	// Scan both nibbles
	xn.accInfoRequest(
		inBusModuleAddr(this->scan_group), false,
	    std::make_unique<Xn::Cb>([this](void *s, void *d) { xnOnInitScanningError(s, d); },
	                             reinterpret_cast<void *>(false))
	);

	if (this->xn.liType() != Xn::LIType::LIUSBEth) {
		// LI-USB-Eth has problems with multiple commands
		xn.accInfoRequest(
			inBusModuleAddr(this->scan_group), true,
			std::make_unique<Xn::Cb>([this](void *s, void *d) { xnOnInitScanningError(s, d); },
			                         reinterpret_cast<void *>(true))
		);
	}
}

void RcsXn::xnOnInitScanningError(void *, void *data) {
	log("Module scanning: no response!", RcsXnLogLevel::llError);
	bool nibble = static_cast<bool>(data);
	this->real_active_in[this->scan_group] = false;
	this->initModuleScanned(this->scan_group, nibble); // continue scanning
}

void RcsXn::initScanningDone() {
	log("Stav vstupů naskenován.", RcsXnLogLevel::llInfo);

	if (rx.s["global"]["mockInputs"].toBool())
		this->real_active_in = this->user_active_in;

	this->started = RcsStartState::started;
	events.call(events.onScanned);

	if (this->s["global"]["resetSignals"].toBool())
		this->resetSignals();
}

void RcsXn::xnSetOutputOk(unsigned int portAddr, int state) {
	if (this->m_acc_op_pending_count > 0)
		this->m_acc_op_pending_count--;

	if (state > 0) {
		static unsigned int id = 0;
		id++;
		if (id == 0)
			id = 1;
		m_accToResetDeq.emplace_back(id, portAddr, QDateTime::currentDateTime().addMSecs(OUTPUT_ACTIVE_TIME));
		m_accToResetArr[portAddr] = id; // so we know which reset time is valid
	}
}

void RcsXn::xnSetOutputError(unsigned int module) {
	// TODO: mark module as failed?
	if (this->m_acc_op_pending_count > 0)
		this->m_acc_op_pending_count--;
	error("Command Station did not respond to SetOutput command!", RCS_MODULE_NOT_ANSWERED_CMD,
	      module);
}

int RcsXn::setPlainOutput(unsigned int portAddr, int state, bool setInternalState) {
	unsigned int module = portAddr / IO_OUT_MODULE_PIN_COUNT;
	unsigned int port = portAddr % IO_OUT_MODULE_PIN_COUNT;
	unsigned int realPortAddr = portAddr;

	if (setInternalState) {
		// Checks only done for non-signal outputs

		unsigned int secondPort = (module<<1) + !(port&1); // 0-2047
		if (state > 0)
			outputs[secondPort] = false;

		outputs[portAddr] = static_cast<bool>(state);
	}

	if (s["global"]["addrRange"].toString() == "lenz") {
		if (module == 0) {
			log("Invalid acc port (using Lenz addresses): " + QString::number(portAddr),
			    RcsXnLogLevel::llWarning);
			return RCS_PORT_INVALID_NUMBER;
		}
		realPortAddr -= IO_OUT_MODULE_PIN_COUNT;
	}

	if ((s["global"]["disableSetOutputOff"].toBool()) && (rx.xn.getTrkStatus() != Xn::TrkStatus::On))
		return RCS_MODULE_INVALID_ADDR;

	this->m_acc_op_pending_count++;
	xn.accOpRequest(
		static_cast<uint16_t>(realPortAddr), static_cast<bool>(state),
		std::make_unique<Xn::Cb>([this, portAddr, state](void *, void *) { this->xnSetOutputOk(portAddr, state); }),
		std::make_unique<Xn::Cb>([this, module](void *, void *) { this->xnSetOutputError(module); })
	);

	if (state == 0)
		m_accToResetArr[portAddr] = 0;

	events.call(events.onOutputChanged, module); // TODO: move to ok callback?
	return 0;
}

template <std::size_t ArraySize>
void RcsXn::parseModules(const QString &active, std::array<bool, ArraySize> &result,
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

	if (s["global"]["addrRange"].toString() == "lenz") {
		// Lenz module 0 (bus) = module 1 (editation)
		if (groupAddr == 255) {
			log("Unsupported acc module (using Lenz addresses): " + QString::number(groupAddr),
				RcsXnLogLevel::llWarning);
			return;
		}
		groupAddr++;
	}

	unsigned int port = 8*groupAddr + 4*nibble;

	this->inputs[port+0] = state.sep.i0;
	this->inputs[port+1] = state.sep.i1;
	this->inputs[port+2] = state.sep.i2;
	this->inputs[port+3] = state.sep.i3;

	if ((this->started == RcsStartState::scanning) && (groupAddr == this->scan_group)) {
		this->real_active_in[groupAddr] = true;
		this->initModuleScanned(groupAddr, nibble);
		return;
	}

	this->real_active_in[groupAddr] = true;

	if (!this->user_active_in[groupAddr]) {
		if (!form.ui.chb_scan_inputs->isChecked())
			return; // no scanning -> ignore change
		this->user_active_in[groupAddr] = true;
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

int RcsXn::setSignal(unsigned int portAddr, unsigned int code) {
	int retval = 0;
	XnSignal &sig = this->sig.at(portAddr/IO_OUT_MODULE_PIN_COUNT);
	sig.currentCode = code;
	if (code < XnSignalCodes.size())
		log(sig.name + ":  " + XnSignalCodes[code], RcsXnLogLevel::llCommands);
	rx.events.call(rx.events.onOutputChanged, sig.hJOPaddr);

	if (sig.tmpl.outputs.find(code) == sig.tmpl.outputs.end()) {
		if (code < XnSignalCodes.size())
			log(sig.name + ": kódu návěsti " + XnSignalCodes[code] +
				" není přiřazen žádný návěstní znak.", RcsXnLogLevel::llWarning);
		return RCS_INVALID_SCOM_CODE;
	}
	QString outputs = sig.tmpl.outputs.at(code);

	for (size_t i = 0; i < std::min(sig.tmpl.outputsCount, static_cast<std::size_t>(outputs.length())); i++) {
		QChar state = outputs[static_cast<int>(i)];
		if (state == 'N')
			continue;

		const unsigned int module = sig.startAddr+i;
		int subret = 0;

		if (state == '+') {
			subret = this->setPlainOutput(2*module + 1, true, false);
		} else if (state == '-') {
			subret = this->setPlainOutput(2*module, true, false);
		} else if (state == '0') {
			subret = this->setPlainOutput(2*module, false, false);
			if (subret != 0 && retval == 0)
				retval = subret;
			subret = this->setPlainOutput(2*module + 1, false, false);
		} else if (state == '1') {
			this->setPlainOutput(2*module, true, false);
			if (subret != 0 && retval == 0)
				retval = subret;
			this->setPlainOutput(2*module + 1, true, false);
		}
		if (subret != 0 && retval == 0)
			retval = subret;
	}

	return retval;
}

bool RcsXn::isResettingSignals() const {
	return this->m_resetSignalsTimer.isActive();
}

void RcsXn::resetSignals() {
	if (this->isResettingSignals())
		return;

	log("Nastavuji návěstidla na stůj...", RcsXnLogLevel::llInfo);
	this->m_resetSignalsIt = this->sig.begin();
	this->m_resetSignalsTimer.start();

	this->resetNextSignal();
}

void RcsXn::resetNextSignal() {
	if (!this->isResettingSignals())
		return;

	if (this->m_resetSignalsIt == this->sig.end() || !this->xn.connected()) {
		this->m_resetSignalsTimer.stop();
		log("Návěstidla nastavena na stůj.", RcsXnLogLevel::llInfo);
		return;
	}

	while (this->m_resetSignalsIt->second.currentCode != 0 && this->m_resetSignalsIt != this->sig.end())
		++this->m_resetSignalsIt;

	if (this->m_resetSignalsIt != this->sig.end()) {
		this->setSignal(this->m_resetSignalsIt->second.startAddr * IO_OUT_MODULE_PIN_COUNT, 0);
		++this->m_resetSignalsIt;
	}
}

void RcsXn::resetIOState() {
	std::fill(this->outputs.begin(), this->outputs.end(), false);
	std::fill(this->inputs.begin(), this->inputs.end(), false);
	for (auto &signal : this->sig)
		signal.second.currentCode = 0;
	std::fill(this->m_accToResetArr.begin(), this->m_accToResetArr.end(), 0);
	this->m_accToResetDeq.clear();
	this->m_acc_op_pending_count = 0;
}

///////////////////////////////////////////////////////////////////////////////

Xn::LIType RcsXn::interface(const QString &name) const {
	if (name == "LI101")
		return Xn::LIType::LI101;
	if (name == "uLI")
		return Xn::LIType::uLI;
	if (name == "LI-USB-Ethernet")
		return Xn::LIType::LIUSBEth;
	return Xn::LIType::LI100;
}

///////////////////////////////////////////////////////////////////////////////

uint8_t RcsXn::inBusModuleAddr(uint8_t userAddr) {
	if (s["global"]["addrRange"].toString() == "lenz") {
		if (userAddr == 0)
			return 0;
		return userAddr - 1;
	}
	return userAddr;
}

///////////////////////////////////////////////////////////////////////////////

void RcsXn::m_acc_reset_timer_tick() {
	/* Only reset of last set-output command should be performed.
	 * But only in situation, when acc command is not pending (because reset would cancel pending set-output)!
	 */

	while ((!this->m_accToResetDeq.empty()) && (QDateTime::currentDateTime() >= this->m_accToResetDeq.front().resetTime)) {
		const AccReset reset = std::move(this->m_accToResetDeq.front());
		this->m_accToResetDeq.pop_front();

		if (reset.id == this->m_accToResetArr[reset.portAddr]) {
			m_accToResetArr[reset.portAddr] = 0;
			if (this->m_accToResetDeq.empty()) {
				this->setPlainOutput(reset.portAddr, 0, false);
			}
		}
	}
}

} // namespace RcsXn
