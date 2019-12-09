/* Implementations of GUI functions from rcs-xn.h */

#include "rcs-xn.h"

void RcsXn::guiInit() {
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
	QObject::connect(form.ui.chb_reset_signals, SIGNAL(stateChanged(int)), this,
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
}

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
	s["global"]["resetSignals"] =
	    (form.ui.chb_reset_signals->checkState() == Qt::CheckState::Checked);

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
