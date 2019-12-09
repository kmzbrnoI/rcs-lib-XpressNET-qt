#include <QMessageBox>
#include <QSerialPortInfo>
#include <QSettings>
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

	// GUI
	form.ui.cb_loglevel->setCurrentIndex(static_cast<int>(this->loglevel));
	QObject::connect(form.ui.cb_loglevel, SIGNAL(currentIndexChanged(int)), this,
	                 SLOT(cb_loglevel_changed(int)));
	QObject::connect(form.ui.cb_interface_type, SIGNAL(currentIndexChanged(int)), this,
	                 SLOT(cb_connections_changed(int)));
	QObject::connect(form.ui.cb_serial_port, SIGNAL(currentIndexChanged(int)), this,
	                 SLOT(cb_connections_changed(int)));
	QObject::connect(form.ui.cb_serial_speed, SIGNAL(currentIndexChanged(int)), this,
	                 SLOT(cb_connections_changed(int)));
	QObject::connect(form.ui.cb_serial_flowcontrol, SIGNAL(currentIndexChanged(int)), this,
	                 SLOT(cb_connections_changed(int)));
	QObject::connect(form.ui.chb_only_one_active, SIGNAL(stateChanged(int)), this,
	                 SLOT(chb_general_config_changed(int)));
	QObject::connect(form.ui.cb_addr_range, SIGNAL(currentIndexChanged(int)), this,
	                 SLOT(chb_general_config_changed(int)));

	QObject::connect(form.ui.b_serial_refresh, SIGNAL(released()), this,
	                 SLOT(b_serial_refresh_handle()));
	QObject::connect(form.ui.b_active_reload, SIGNAL(released()), this,
	                 SLOT(b_active_load_handle()));
	QObject::connect(form.ui.b_active_save, SIGNAL(released()), this, SLOT(b_active_save_handle()));

	QObject::connect(form.ui.b_signal_add, SIGNAL(released()), this, SLOT(b_signal_add_handle()));
	QObject::connect(form.ui.b_signal_remove, SIGNAL(released()), this,
	                 SLOT(b_signal_remove_handle()));

	QObject::connect(form.ui.tw_xn_log, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this,
	                 SLOT(tw_log_double_clicked(QTreeWidgetItem *, int)));
	QObject::connect(form.ui.tw_signals, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this,
	                 SLOT(tw_signals_dbl_click(QTreeWidgetItem *, int)));
	QObject::connect(form.ui.tw_signals, SIGNAL(itemSelectionChanged()), this,
	                 SLOT(tw_signals_selection_changed()));

	QObject::connect(form.ui.b_dcc_on, SIGNAL(released()), this, SLOT(b_dcc_on_handle()));
	QObject::connect(form.ui.b_dcc_off, SIGNAL(released()), this, SLOT(b_dcc_off_handle()));

	QString text;
	text.sprintf("Nastavení RCS XpressNET knihovny v%d.%d", VERSION_MAJOR, VERSION_MINOR);
	form.setWindowTitle(text);
	form.setFixedSize(form.size());

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

	events.call(rx.events.beforeStart);
	started = RcsStartState::scanning;
	events.call(rx.events.afterStart);
	this->first_scan();
	return 0;
}

int RcsXn::stop() {
	if (rx.started == RcsStartState::stopped)
		return RCS_NOT_STARTED;

	events.call(rx.events.beforeStop);
	this->started = RcsStartState::stopped;
	events.call(rx.events.afterStop);
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
			this->loadSignals(filename);
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
	this->scan_group = 0;
	this->scan_nibble = false;
	xn.accInfoRequest(
		scan_group, scan_nibble,
		std::make_unique<Xn::Cb>([this](void *s, void *d) { xnOnInitScanningError(s, d); })
	);
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
	for (size_t i = 0; i < IO_MODULES_COUNT; i++) {
		if (this->active_in[i])
			this->in_count++;
		if (this->active_out[i])
			this->out_count++;
		if (this->active_in[i] || this->active_out[i])
			this->modules_count++;
	}

	if ((s["global"]["addrRange"].toString() == "lenz") && (this->active_in[0] || this->active_out[0]))
		throw EInvalidRange("Adresa 0 není validní adresou systému Lenz!");
}

void RcsXn::initModuleScanned(uint8_t group, bool nibble) {
	// Pick next address
	unsigned next_module = (group*4) + (nibble*2) + 2; // LSB always 0!

	unsigned correction = 0;
	if (s["global"]["addrRange"].toString() == "lenz")
		correction += 1;

	while ((next_module < IO_MODULES_COUNT) && (!this->active_in[next_module+correction]) &&
	       (!this->active_in[next_module+correction+1]))
		next_module += 2;

	if (next_module >= IO_MODULES_COUNT) {
		this->initScanningDone();
		return;
	}

	this->scan_group = static_cast<uint8_t>(next_module/4);
	this->scan_nibble = (next_module%4) >> 1;

	xn.accInfoRequest(
	    this->scan_group, this->scan_nibble,
	    std::make_unique<Xn::Cb>([this](void *s, void *d) { xnOnInitScanningError(s, d); })
	);
}

void RcsXn::xnOnInitScanningError(void *, void *) {
	error("Module scanning: no response!", RCS_NOT_OPENED);
	this->stop();
}

void RcsXn::initScanningDone() {
	this->started = RcsStartState::started;
	events.call(events.onScanned);
}

void RcsXn::xnSetOutputError(void *sender, void *data) {
	(void)sender;
	// TODO: mark module as failed?
	unsigned int module = reinterpret_cast<uintptr_t>(data);
	error("Command Station did not respond to SetOutput command!", RCS_MODULE_NOT_ANSWERED_CMD,
	      module);
}

int RcsXn::setPlainOutput(unsigned int portAddr, int state) {
	unsigned int module = portAddr / IO_MODULE_PIN_COUNT;
	unsigned int port = portAddr % IO_MODULE_PIN_COUNT;

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
		realPortAddr -= IO_MODULE_PIN_COUNT;
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
	} else if (s == Xn::TrkStatus::Off) {
		form.ui.l_dcc_state->setText("PROGRAM");
		widgetSetColor(*form.ui.l_dcc_state, Qt::yellow);
	} else {
		form.ui.l_dcc_state->setText("???");
		widgetSetColor(*form.ui.l_dcc_state, Qt::black);
	}

	if (this->opening) {
		if (s == Xn::TrkStatus::Off) {
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

	if (rx.s["global"]["addrRange"].toString() == "lenz")
		port += IO_MODULE_PIN_COUNT;

	this->inputs[port+0] = state.sep.i0;
	this->inputs[port+1] = state.sep.i1;
	this->inputs[port+2] = state.sep.i2;
	this->inputs[port+3] = state.sep.i3;

	if ((this->started == RcsStartState::scanning) && (groupAddr == this->scan_group) &&
		(nibble == this->scan_nibble)) {
		this->initModuleScanned(groupAddr, nibble);
		return;
	}

	const std::vector<unsigned int> activeCheck = {port/2, (port/2)+1};
	bool anyActivated = false;
	for (const auto& activeAddr : activeCheck) {
		if (!this->active_in[activeAddr]) {
			if (!form.ui.chb_scan_inputs->isChecked())
				return; // no scanning -> ignore change
			this->active_in[activeAddr] = true;
			this->modules_count++;
			this->in_count++;
			anyActivated = true;
		}
	}
	if (anyActivated) {
		try { this->fillActiveIO(); } catch (...) {}
	}

	events.call(events.onInputChanged, port/2);
	events.call(events.onInputChanged, (port/2)+1);
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

void RcsXn::loadSignals(const QString &filename) {
	QSettings s(filename, QSettings::IniFormat);
	s.setIniCodec("UTF-8");

	// signals
	for (const auto &g : s.childGroups()) {
		if (not g.startsWith("Signal"))
			continue;

		try {
			QStringList name = g.split('-');
			if (name.size() < 2) {
				log("Invalid signal: " + g, RcsXnLogLevel::llWarning);
				continue;
			}

			unsigned int hJOPoutput = name[1].toUInt(); // signal always at nibble 0

			s.beginGroup(g);
			this->sig.emplace(hJOPoutput, XnSignal(s, hJOPoutput));
			s.endGroup();
		} catch (...) {
			log("Invalid signal: " + g, RcsXnLogLevel::llWarning);
		}
	}

	// signal templates
	for (const auto &g : s.childGroups()) {
		if (not g.startsWith("SigTemplate"))
			continue;

		try {
			QStringList name = g.split('-');
			if (name.size() < 2) {
				log("Invalid signal template: " + g, RcsXnLogLevel::llWarning);
				continue;
			}

			const QString sigName = name[1];

			s.beginGroup(g);
			this->sigTemplates.emplace(sigName, XnSignalTemplate(s));
			s.endGroup();
		} catch (...) {
			log("Invalid signal template: " + g, RcsXnLogLevel::llWarning);
		}
	}
}

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

	// signals
	for (const auto &pair : this->sig) {
		QString group = "Signal-" + QString::number(pair.first);
		s.beginGroup(group);
		pair.second.saveData(s);
		s.endGroup();
	}

	// signal templates
	for (const auto &pair : this->sigTemplates) {
		s.beginGroup("SigTemplate-" + pair.first);
		pair.second.saveData(s);
		s.endGroup();
	}
}

bool RcsXn::isSignal(unsigned int portAddr) const {
	return ((!(portAddr&1)) && (this->sig.find(portAddr >> 1) != this->sig.end()));
}

void RcsXn::setSignal(unsigned int portAddr, unsigned int code) {
	XnSignal &sig = this->sig.at(portAddr/IO_MODULE_PIN_COUNT);
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

void RcsXn::resetIOState() {
	std::fill(this->outputs.begin(), this->outputs.end(), false);
	std::fill(this->inputs.begin(), this->inputs.end(), false);
	for (auto &signal : this->sig)
		signal.second.currentCode = 0;
}

///////////////////////////////////////////////////////////////////////////////
// UI slots

void RcsXn::cb_loglevel_changed(int index) { this->setLogLevel(static_cast<RcsXnLogLevel>(index)); }

void RcsXn::cb_connections_changed(int) {
	if (this->gui_config_changing)
		return;

	s["XN"]["interface"] = form.ui.cb_interface_type->currentText();
	s["XN"]["port"] = form.ui.cb_serial_port->currentText();
	s["XN"]["baudrate"] = form.ui.cb_serial_speed->currentText().toInt();
	s["XN"]["flowcontrol"] = form.ui.cb_serial_flowcontrol->currentIndex();
}

void RcsXn::fillConnectionsCbs() {
	// Interface type
	form.ui.cb_interface_type->setCurrentText(s["XN"]["interface"].toString());

	// Port
	this->fillPortCb();

	this->gui_config_changing = true;

	// Speed
	form.ui.cb_serial_speed->clear();
	bool is_item = false;
	for (const qint32 &br : QSerialPortInfo::standardBaudRates()) {
		form.ui.cb_serial_speed->addItem(QString::number(br));
		if (br == s["XN"]["baudrate"].toInt())
			is_item = true;
	}
	if (is_item)
		form.ui.cb_serial_speed->setCurrentText(s["XN"]["baudrate"].toString());
	else
		form.ui.cb_serial_speed->setCurrentIndex(-1);

	// Flow control
	form.ui.cb_serial_flowcontrol->setCurrentIndex(s["XN"]["flowcontrol"].toInt());

	this->gui_config_changing = false;
}

void RcsXn::fillPortCb() {
	this->gui_config_changing = true;

	form.ui.cb_serial_port->clear();
	bool is_item = false;
	for (const QSerialPortInfo &port : QSerialPortInfo::availablePorts()) {
		form.ui.cb_serial_port->addItem(port.portName());
		if (port.portName() == s["XN"]["port"].toString())
			is_item = true;
	}
	if (is_item)
		form.ui.cb_serial_port->setCurrentText(s["XN"]["port"].toString());
	else
		form.ui.cb_serial_port->setCurrentIndex(-1);

	this->gui_config_changing = false;
}

void RcsXn::b_serial_refresh_handle() { this->fillPortCb(); }

void RcsXn::guiOnOpen() {
	form.ui.cb_interface_type->setEnabled(false);
	form.ui.cb_serial_port->setEnabled(false);
	form.ui.cb_serial_speed->setEnabled(false);
	form.ui.cb_serial_flowcontrol->setEnabled(false);
	form.ui.b_serial_refresh->setEnabled(false);

	form.ui.b_active_save->setEnabled(false);
	form.ui.te_active_inputs->setEnabled(false);
	form.ui.te_active_outputs->setEnabled(false);

	form.ui.b_dcc_on->setEnabled(true);
	form.ui.b_dcc_off->setEnabled(true);

	this->fillActiveIO();
}

void RcsXn::guiOnClose() {
	form.ui.cb_interface_type->setEnabled(true);
	form.ui.cb_serial_port->setEnabled(true);
	form.ui.cb_serial_speed->setEnabled(true);
	form.ui.cb_serial_flowcontrol->setEnabled(true);
	form.ui.b_serial_refresh->setEnabled(true);

	form.ui.b_active_save->setEnabled(true);
	form.ui.te_active_inputs->setEnabled(true);
	form.ui.te_active_outputs->setEnabled(true);

	form.ui.b_dcc_on->setEnabled(false);
	form.ui.b_dcc_off->setEnabled(false);
	form.ui.l_dcc_state->setText("???");
	widgetSetColor(*form.ui.l_dcc_state, Qt::black);
}

void RcsXn::b_active_load_handle() {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	this->fillActiveIO();
	QApplication::restoreOverrideCursor();
	QMessageBox::information(&(this->form), "Ok", "Načteno.", QMessageBox::Ok);
}

void RcsXn::b_active_save_handle() {
	QApplication::setOverrideCursor(Qt::WaitCursor);

	try {
		this->loadActiveIO(
			form.ui.te_active_inputs->toPlainText().replace("\n", ","),
			form.ui.te_active_outputs->toPlainText().replace("\n", ",")
		);
		this->saveConfig(this->config_filename);
		QApplication::restoreOverrideCursor();
		QMessageBox::information(&(this->form), "Ok", "Uloženo.", QMessageBox::Ok);
	} catch (const EInvalidRange &e) {
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(&(this->form), "Chyba!", "Zadán neplatný rozsah:\n" + e.str(),
		                     QMessageBox::Ok);
	} catch (const QStrException &e) {
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(&(this->form), "Chyba!", e.str(), QMessageBox::Ok);
	} catch (...) {
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(&(this->form), "Chyba!", "Neznámá chyba.", QMessageBox::Ok);
	}
}

void RcsXn::fillActiveIO() {
	form.ui.te_active_inputs->setText(getActiveStr(this->active_in, ",\n"));
	form.ui.te_active_outputs->setText(getActiveStr(this->active_out, ",\n"));
	form.ui.l_io_count->setText(QString::number(this->in_count) + " vstupů, " +
	                            QString::number(this->out_count) + " výstupů");
}

void RcsXn::tw_log_double_clicked(QTreeWidgetItem *item, int column) {
	(void)item;
	(void)column;
	this->form.ui.tw_xn_log->clear();
}

void RcsXn::b_signal_add_handle() {
	f_signal_edit.open([this](XnSignal signal) { this->newSignal(signal); }, this->sigTemplates);
}

void RcsXn::b_signal_remove_handle() {
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(&(this->form), "Smazat?", "Skutečně smazat vybraná návěstidla?",
                                  QMessageBox::Yes|QMessageBox::No);
	if (reply != QMessageBox::Yes)
		return;

	QApplication::setOverrideCursor(Qt::WaitCursor);

	for (const QTreeWidgetItem *item : form.ui.tw_signals->selectedItems()) {
		unsigned int hJOPaddr = item->text(0).toUInt();
		this->sig.erase(hJOPaddr);

		// this is slow, but I found no other way :(
		for (int i = 0; i < form.ui.tw_signals->topLevelItemCount(); ++i)
			if (form.ui.tw_signals->topLevelItem(i) == item)
				delete form.ui.tw_signals->takeTopLevelItem(i);
	}
	this->saveSignals(this->config_filename);

	QApplication::restoreOverrideCursor();
}

void RcsXn::fillSignals() {
	form.ui.tw_signals->setSortingEnabled(false);
	form.ui.tw_signals->clear();
	for (const auto &signal_tuple : this->sig)
		this->guiAddSignal(signal_tuple.second);
	form.ui.tw_signals->setSortingEnabled(true);
	form.ui.tw_signals->sortByColumn(0, Qt::SortOrder::AscendingOrder);
}

void RcsXn::guiAddSignal(const XnSignal &signal) {
	auto *item = new QTreeWidgetItem(form.ui.tw_signals);
	item->setText(0, QString::number(signal.hJOPaddr));
	item->setText(1, signal.outputRange());
	item->setText(2, signal.name);
	form.ui.tw_signals->addTopLevelItem(item);
}

void RcsXn::newSignal(XnSignal signal) {
	if (this->sig.find(signal.hJOPaddr) != this->sig.end())
		throw QStrException("Návěstidlo s touto hJOP adresou je již definováno!");
	this->sig.emplace(signal.hJOPaddr, signal);
	this->guiAddSignal(signal);
	this->saveSignals(this->config_filename);
}

void RcsXn::editedSignal(XnSignal signal) {
	if ((signal.hJOPaddr != this->current_editing_signal) &&
	    (this->sig.find(signal.hJOPaddr) != this->sig.end()))
		throw QStrException("Návěstidlo s touto hJOP adresou je již definováno!");
	if (this->sig.find(this->current_editing_signal) != this->sig.end()) {
		this->sig.erase(this->current_editing_signal);
		for (int i = 0; i < form.ui.tw_signals->topLevelItemCount(); ++i)
			if (form.ui.tw_signals->topLevelItem(i)->text(0).toUInt() ==
			    this->current_editing_signal)
				delete form.ui.tw_signals->takeTopLevelItem(i);
	}
	this->sig.emplace(signal.hJOPaddr, signal);
	this->guiAddSignal(signal);
	this->saveSignals(this->config_filename);
}

void RcsXn::tw_signals_dbl_click(QTreeWidgetItem *item, int column) {
	(void)column;
	this->current_editing_signal = item->text(0).toUInt();
	f_signal_edit.open(this->sig[this->current_editing_signal],
	                   [this](XnSignal signal) { this->editedSignal(signal); }, this->sigTemplates);
}

void RcsXn::tw_signals_selection_changed() {
	form.ui.b_signal_remove->setEnabled(!form.ui.tw_signals->selectedItems().empty());
}

void RcsXn::chb_general_config_changed(int) {
	if (this->gui_config_changing)
		return;

	s["global"]["onlyOneActive"] =
	    (form.ui.chb_only_one_active->checkState() == Qt::CheckState::Checked);

	if (form.ui.cb_addr_range->currentIndex() == 0)
		s["global"]["addrRange"] = "basic";
	else if (form.ui.cb_addr_range->currentIndex() == 1)
		s["global"]["addrRange"] = "lenz";
}

void RcsXn::xn_onDccError(void *, void *) {
	form.ui.b_dcc_on->setEnabled(true);
	form.ui.b_dcc_off->setEnabled(true);
	QMessageBox::warning(&(this->form), "Error!",
	                     "Centrála neodpověděla na příkaz o nastavení DCC!");
}

void RcsXn::xn_onDccOpenError(void *, void *) {
	log("No response on 'Set DCC' command!", RcsXnLogLevel::llError);
	this->close();
}

void RcsXn::setDcc(Xn::TrkStatus status) {
	try {
		if (xn.connected())
			xn.setTrkStatus(
				status, nullptr,
				std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_onDccError(s, d); })
			);
	} catch (const Xn::QStrException& e) {
		form.ui.b_dcc_on->setEnabled(true);
		form.ui.b_dcc_off->setEnabled(true);
		QMessageBox::warning(&(this->form), "Chyba!", e.str(), QMessageBox::Ok);
	}
}

void RcsXn::b_dcc_on_handle() {
	form.ui.b_dcc_on->setEnabled(false);
	this->setDcc(Xn::TrkStatus::On);
}
void RcsXn::b_dcc_off_handle() {
	form.ui.b_dcc_off->setEnabled(false);
	this->setDcc(Xn::TrkStatus::Off);
}

void RcsXn::widgetSetColor(QWidget &widget, const QColor &color) {
	QPalette palette = widget.palette();
	palette.setColor(QPalette::WindowText, color);
	widget.setPalette(palette);
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
