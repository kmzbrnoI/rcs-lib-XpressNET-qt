#include <QMessageBox>
#include <QSerialPortInfo>

#include "rcs-xn.h"
#include "q-tree-num-widget-item.h"

/* Implementations of GUI functions from rcs-xn.h */

namespace RcsXn {

void RcsXn::guiInit() {
	form.ui.cb_loglevel->setCurrentIndex(static_cast<int>(this->loglevel));
	QObject::connect(form.ui.cb_loglevel, SIGNAL(currentIndexChanged(int)), this,
	                 SLOT(cb_loglevel_changed(int)));
	QObject::connect(form.ui.cb_interface_type, SIGNAL(currentIndexChanged(int)), this,
					 SLOT(cb_interface_type_changed(int)));
	QObject::connect(form.ui.cb_serial_port, SIGNAL(currentIndexChanged(int)), this,
	                 SLOT(cb_connections_changed(int)));
	QObject::connect(form.ui.cb_serial_speed, SIGNAL(currentIndexChanged(int)), this,
	                 SLOT(cb_connections_changed(int)));
	QObject::connect(form.ui.cb_serial_flowcontrol, SIGNAL(currentIndexChanged(int)), this,
	                 SLOT(cb_connections_changed(int)));
	QObject::connect(form.ui.chb_reset_signals, SIGNAL(stateChanged(int)), this,
	                 SLOT(chb_general_config_changed(int)));
	QObject::connect(form.ui.chb_disable_set_output_off, SIGNAL(stateChanged(int)), this,
					 SLOT(chb_general_config_changed(int)));
	QObject::connect(form.ui.cb_addr_range, SIGNAL(currentIndexChanged(int)), this,
	                 SLOT(chb_general_config_changed(int)));

	QObject::connect(form.ui.b_serial_refresh, SIGNAL(released()), this,
	                 SLOT(b_serial_refresh_handle()));
	QObject::connect(form.ui.b_active_reload, SIGNAL(released()), this,
	                 SLOT(b_active_load_handle()));
	QObject::connect(form.ui.b_active_save, SIGNAL(released()), this, SLOT(b_active_save_handle()));

	QObject::connect(form.ui.b_apply_binary, SIGNAL(released()), this, SLOT(b_apply_binary_handle()));

	QObject::connect(form.ui.b_signal_add, SIGNAL(released()), this, SLOT(b_signal_add_handle()));
	QObject::connect(form.ui.b_signal_remove, SIGNAL(released()), this,
	                 SLOT(b_signal_remove_handle()));

	QObject::connect(form.ui.tw_xn_log, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this,
	                 SLOT(tw_log_double_clicked(QTreeWidgetItem*,int)));
	QObject::connect(form.ui.tw_signals, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this,
	                 SLOT(tw_signals_dbl_click(QTreeWidgetItem*,int)));
	QObject::connect(form.ui.tw_signals, SIGNAL(itemSelectionChanged()), this,
	                 SLOT(tw_signals_selection_changed()));

	QObject::connect(&this->f_module_edit, SIGNAL(accepted()), this,
	                 SLOT(f_module_edit_accepted()));
	QObject::connect(form.ui.tw_input_modules, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this,
	                 SLOT(tw_input_modules_dbl_click(QTreeWidgetItem*,int)));

	QObject::connect(form.ui.b_dcc_on, SIGNAL(released()), this, SLOT(b_dcc_on_handle()));
	QObject::connect(form.ui.b_dcc_off, SIGNAL(released()), this, SLOT(b_dcc_off_handle()));

	form.ui.tw_main->setCurrentIndex(0);

#ifdef RCS_XN_RELEASE
	QString text = QString::asprintf("Nastavení RCS XpressNET knihovny v%d.%d", VERSION_MAJOR, VERSION_MINOR);
#else
	QString text = QString::asprintf("Nastavení RCS XpressNET knihovny v%d.%d-dev", VERSION_MAJOR, VERSION_MINOR);
#endif
	form.setWindowTitle(text);
	form.setWindowFlags(Qt::Dialog);
}

void RcsXn::cb_loglevel_changed(int index) { this->setLogLevel(static_cast<RcsXnLogLevel>(index)); }

void RcsXn::cb_interface_type_changed(int arg) {
	this->cb_connections_changed(arg);
	if ((s["XN"]["port"].toString() == "auto") && (form.ui.cb_interface_type->currentText() != "uLI"))
		s["XN"]["port"] = "";
	this->fillPortCb();
}

void RcsXn::cb_connections_changed(int) {
	if (this->gui_config_changing)
		return;

	s["XN"]["interface"] = form.ui.cb_interface_type->currentText();
	s["XN"]["baudrate"] = form.ui.cb_serial_speed->currentText().toInt();
	s["XN"]["flowcontrol"] = form.ui.cb_serial_flowcontrol->currentIndex();

	const QString port = form.ui.cb_serial_port->currentText();
	s["XN"]["port"] = (port.startsWith("Auto")) ? "auto" : port;
}

void RcsXn::fillConnectionsCbs() {
	this->gui_config_changing = true;

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

	if (form.ui.cb_interface_type->currentText() == "uLI") {
		form.ui.cb_serial_port->addItem("Automaticky detekovat port uLI");
		if (s["XN"]["port"].toString() == "auto") {
			is_item = true;
			form.ui.cb_serial_port->setCurrentIndex(0);
		}
	}

	const auto& ports = QSerialPortInfo::availablePorts();
	for (const QSerialPortInfo &port : ports) {
		form.ui.cb_serial_port->addItem(port.portName());
		if (port.portName() == s["XN"]["port"].toString())
			is_item = true;
	}

	if (s["XN"]["port"].toString() != "auto") {
		if (is_item)
			form.ui.cb_serial_port->setCurrentText(s["XN"]["port"].toString());
		else
			form.ui.cb_serial_port->setCurrentIndex(-1);
	}

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
	this->f_module_edit.setActiveEnabled(false);
	form.ui.te_active_outputs->setEnabled(false);

	form.ui.b_dcc_on->setEnabled(true);
	form.ui.b_dcc_off->setEnabled(true);

	this->fillActiveOutputs();
}

void RcsXn::guiOnClose() {
	form.ui.cb_interface_type->setEnabled(true);
	form.ui.cb_serial_port->setEnabled(true);
	form.ui.cb_serial_speed->setEnabled(true);
	form.ui.cb_serial_flowcontrol->setEnabled(true);
	form.ui.b_serial_refresh->setEnabled(true);

	form.ui.b_active_save->setEnabled(true);
	this->f_module_edit.setActiveEnabled(true);
	form.ui.te_active_outputs->setEnabled(true);

	form.ui.b_dcc_on->setEnabled(false);
	form.ui.b_dcc_off->setEnabled(false);
	form.ui.l_dcc_state->setText("???");
	widgetSetColor(*form.ui.l_dcc_state, Qt::black);
}

void RcsXn::b_active_load_handle() {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	this->fillActiveOutputs();
	QApplication::restoreOverrideCursor();
	QMessageBox::information(&(this->form), "Ok", "Načteno.", QMessageBox::Ok);
}

void RcsXn::b_active_save_handle() {
	QApplication::setOverrideCursor(Qt::WaitCursor);

	try {
		this->loadActiveIO("", form.ui.te_active_outputs->toPlainText().replace("\n", ","));
		this->saveConfig();
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

void RcsXn::fillActiveOutputs() {
	form.ui.te_active_outputs->setText(getActiveStr(this->user_active_out, ",\n"));
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
	this->saveConfig();

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
	auto *item = new FirstNumTreeWidgetItem(form.ui.tw_signals);
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
	this->saveConfig();
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
	this->saveConfig();
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

	s["global"]["resetSignals"] =
	    (form.ui.chb_reset_signals->checkState() == Qt::CheckState::Checked);
	s["global"]["disableSetOutputOff"] =
		(form.ui.chb_disable_set_output_off->checkState() == Qt::CheckState::Checked);

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

void RcsXn::b_apply_binary_handle() {
	QApplication::setOverrideCursor(Qt::WaitCursor);

	try {
		this->parseModules(form.ui.te_binary_outputs->toPlainText().replace("\n", ","), this->binary, true);
		this->saveConfig();
		form.ui.te_binary_outputs->setText(getActiveStr(this->binary, ",\n"));
		QApplication::restoreOverrideCursor();
		QMessageBox::information(&(this->form), "Ok", "Použito.", QMessageBox::Ok);
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

void RcsXn::twFillInputModules() {
	form.ui.tw_input_modules->clear();

	for (unsigned addr = 0; addr < IO_IN_MODULES_COUNT; addr++) {
		auto *item = new FirstNumTreeWidgetItem(form.ui.tw_input_modules);
		form.ui.tw_input_modules->addTopLevelItem(item);
		this->twUpdateInputModule(addr);
	}

	for (int i = 0; i < form.ui.tw_input_modules->columnCount(); ++i)
		form.ui.tw_input_modules->resizeColumnToContents(i);
}

void RcsXn::twUpdateInputModule(unsigned addr) {
	QTreeWidgetItem *item = this->form.ui.tw_input_modules->topLevelItem(addr);
	if ((item == nullptr) || (addr >= this->modules_in.size()))
		return;
	const RcsInputModule& module = this->modules_in[addr];

	item->setText(0, QString::number(addr));
	item->setText(1, module.wantActive ? "✓" : "");
	item->setText(2, module.name);

	{
		QString delays = "";
		for (unsigned i = 0; i < module.inputFallDelays.size(); i++) {
			if (i == (module.inputFallDelays.size()/2))
				delays += "   ";
			delays += RcsInputModule::fallDelayToStr(module.inputFallDelays[i]) + "s";
			if (i < (module.inputFallDelays.size()-1))
				delays += " ";
		}
		item->setText(3, delays);
	}

	this->twUpdateInputModuleInputs(addr);
}

void RcsXn::twUpdateInputModuleInputs(unsigned addr) {
	QTreeWidgetItem *item = this->form.ui.tw_input_modules->topLevelItem(addr);
	if ((item == nullptr) || (addr >= this->modules_in.size()))
		return;
	const RcsInputModule& module = this->modules_in[addr];

	QString state = "";
	for (unsigned i = 0; i < module.state.size(); i++) {
		if (i == (module.state.size()/2))
			state += " ";

		if (!module.realActive)
			state += "-";
		else if (module.state[i] == XnInState::on)
			state += "1";
		else if (module.state[i] == XnInState::off)
			state += "0";
		else if (module.state[i] == XnInState::falling)
			state += "v";
		else
			state += "?";
	}
	item->setText(4, state);
}

void RcsXn::tw_input_modules_dbl_click(QTreeWidgetItem *item, int column) {
	(void)column;
	f_module_edit.moduleOpen(&this->modules_in[this->form.ui.tw_input_modules->indexOfTopLevelItem(item)]);
}

void RcsXn::f_module_edit_accepted() {
	if (this->f_module_edit.module == nullptr)
		return;
	const unsigned moduleAddr = this->f_module_edit.module->addr;

	this->twUpdateInputModule(moduleAddr);
	this->refreshActiveIOCounts();
	rx.events.call(rx.events.onModuleChanged, moduleAddr);
	this->saveConfig();
}

} // namespace RcsXn
