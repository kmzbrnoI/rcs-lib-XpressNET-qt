#include "form-signal-edit.h"
#include "lib/q-str-exception.h"
#include <QMessageBox>
#include <cmath>

namespace SignalEdit {

FormSignalEdit::FormSignalEdit(TmplStorage &templates, QWidget *parent)
    : QDialog(parent), templates(templates) {
	ui.setupUi(this);
	this->setFixedSize(this->size());

	for (size_t i = 0; i < RcsXn::XnSignalCodes.size(); ++i)
		ui.cb_add_sig->addItem(QString::number(i) + ": " + RcsXn::XnSignalCodes[i]);

	QObject::connect(ui.b_apply, SIGNAL(released()), this, SLOT(b_apply_handle()));
	QObject::connect(ui.b_storno, SIGNAL(released()), this, SLOT(b_storno_handle()));
	QObject::connect(ui.b_temp_load, SIGNAL(released()), this, SLOT(b_temp_load_handle()));
	QObject::connect(ui.b_delete_signal, SIGNAL(released()), this, SLOT(b_delete_signal_handle()));
	QObject::connect(ui.b_add_signal, SIGNAL(released()), this, SLOT(b_add_signal_handle()));
	QObject::connect(ui.b_temp_save, SIGNAL(released()), this, SLOT(b_temp_save_handle()));
	QObject::connect(ui.tw_outputs, SIGNAL(itemSelectionChanged()), this,
	                 SLOT(tw_outputs_selection_changed()));
	QObject::connect(ui.sb_output_count, SIGNAL(valueChanged(int)), this,
	                 SLOT(sb_output_count_changed(int)));
	QObject::connect(ui.sb_hjop_rcs_addr, SIGNAL(valueChanged(int)), this,
					 SLOT(sb_hjop_rcs_add_changed(int)));
}

void FormSignalEdit::open(EditCallback callback, TmplStorage &templates) {
	this->callback = callback;
	this->templates = templates;
	ui.b_delete_signal->setEnabled(false);

	ui.le_name->setText("");
	ui.sb_hjop_rcs_addr->setValue(0);
	ui.sb_output_addr->setValue(0);
	ui.sb_output_count->setValue(4);
	ui.tw_outputs->clear();
	ui.sb_add_bits->setValue(0);

	this->fillTemplates(templates);
	this->setWindowTitle("Přidat nové návěstidlo");
	ui.le_name->setFocus();
	this->show();
}

void FormSignalEdit::open(RcsXn::XnSignal signal, EditCallback callback, TmplStorage &templates) {
	this->callback = callback;
	this->templates = templates;
	ui.b_delete_signal->setEnabled(false);

	ui.le_name->setText(signal.name);
	ui.sb_hjop_rcs_addr->setValue(signal.hJOPaddr);
	ui.sb_output_addr->setValue(signal.startAddr);
	ui.sb_output_count->setValue(signal.tmpl.outputsCount);

	this->fillTemplate(signal.tmpl);

	ui.sb_add_bits->setValue(0);

	this->fillTemplates(templates);
	this->setWindowTitle("Upravit návěstidlo " + signal.name);
	ui.le_name->setFocus();
	this->show();
}

void FormSignalEdit::fillTemplate(const RcsXn::XnSignalTemplate &tmpl) {
	ui.tw_outputs->setSortingEnabled(false);
	ui.tw_outputs->clear();
	for (const auto &output : tmpl.outputs) {
		auto *item = new QTreeWidgetItem(ui.tw_outputs);
		item->setText(0, QString::number(output.first));
		if (output.first < RcsXn::XnSignalCodes.size())
			item->setText(1, RcsXn::XnSignalCodes[output.first]);
		else
			item->setText(1, "?");
		item->setText(2, QString::number(output.second, 2).rightJustified(tmpl.outputsCount, '0'));
		ui.tw_outputs->addTopLevelItem(item);
	}
	ui.tw_outputs->setSortingEnabled(true);
	ui.tw_outputs->sortByColumn(0, Qt::SortOrder::AscendingOrder);
	for (int i = 0; i < ui.tw_outputs->columnCount(); ++i)
		ui.tw_outputs->resizeColumnToContents(i);
}

RcsXn::XnSignalTemplate FormSignalEdit::getTemplate() const {
	RcsXn::XnSignalTemplate result;
	result.outputsCount = ui.sb_output_count->value();
	for (int i = 0; i < ui.tw_outputs->topLevelItemCount(); ++i) {
		const QTreeWidgetItem *item = ui.tw_outputs->topLevelItem(i);
		result.outputs.emplace(item->text(0).toUInt(), item->text(2).toUInt(nullptr, 2));
	}
	return result;
}

void FormSignalEdit::b_apply_handle() {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();

	RcsXn::XnSignal result;

	result.name = ui.le_name->text();
	result.hJOPaddr = ui.sb_hjop_rcs_addr->value();
	result.startAddr = ui.sb_output_addr->value();
	result.tmpl = this->getTemplate();

	try {
		if (this->callback != nullptr)
			this->callback(result);
		this->callback = nullptr;
		QApplication::restoreOverrideCursor();
		this->close();
	} catch (const RcsXn::QStrException &e) {
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, "Chyba", e.str(), QMessageBox::Ok);
	}
}

void FormSignalEdit::b_storno_handle() {
	this->callback = nullptr;
	this->close();
}

void FormSignalEdit::b_temp_load_handle() {
	if (ui.cb_temp_load->currentIndex() < 0) {
		QMessageBox::warning(this, "Chyba", "Je třeba vybrat šablonu!", QMessageBox::Ok);
		return;
	}

	this->fillTemplate(this->templates.at(ui.cb_temp_load->currentText()));
	ui.le_temp_save_name->setText(ui.cb_temp_load->currentText());
	QMessageBox::information(this, "Ok", "Šablona " + ui.cb_temp_load->currentText() + " načtena.",
	                         QMessageBox::Ok);
}

void FormSignalEdit::b_delete_signal_handle() {
	QMessageBox::StandardButton reply = QMessageBox::question(
	    this, "Smazat?", "Skutečné smazat vybrané návěsti?", QMessageBox::Yes | QMessageBox::No
	);
	if (reply != QMessageBox::Yes)
		return;

	for (const QTreeWidgetItem *item : ui.tw_outputs->selectedItems()) {
		// this is slow, but I found no other way :(
		for (int i = 0; i < ui.tw_outputs->topLevelItemCount(); ++i)
			if (ui.tw_outputs->topLevelItem(i) == item)
				delete ui.tw_outputs->takeTopLevelItem(i);
	}
}

void FormSignalEdit::b_add_signal_handle() {
	if (ui.cb_add_sig->currentIndex() < 0) {
		QMessageBox::warning(this, "Chyba", "Je třeba vybrat návěst!", QMessageBox::Ok);
		return;
	}

	for (int i = 0; i < ui.tw_outputs->topLevelItemCount(); ++i) {
		if (ui.tw_outputs->topLevelItem(i)->text(0).toInt() == ui.cb_add_sig->currentIndex()) {
			// edit output
			ui.tw_outputs->topLevelItem(i)->setText(
			    2,
			    QString::number(ui.sb_add_bits->value(), 2).rightJustified(ui.sb_output_count->value(), '0')
			);
			return;
		}
	}

	// new output
	auto *item = new QTreeWidgetItem(ui.tw_outputs);
	item->setText(0, QString::number(ui.cb_add_sig->currentIndex()));
	item->setText(1, RcsXn::XnSignalCodes[ui.cb_add_sig->currentIndex()]);
	item->setText(2, QString::number(ui.sb_add_bits->value(), 2)
	                     .rightJustified(ui.sb_output_count->value(), '0'));
	ui.tw_outputs->addTopLevelItem(item);
}

void FormSignalEdit::b_temp_save_handle() {
	const QString name = ui.le_temp_save_name->text();

	if (name == "") {
		QMessageBox::warning(this, "Chyba", "Je třeba vybrat pojmenování šablony!",
		                     QMessageBox::Ok);
		return;
	}

	if (this->templates.find(name) != this->templates.end()) {
		QMessageBox::StandardButton reply = QMessageBox::question(
		    this, "Přepsat?", "Přepsat uloženou šablonu " + name + "?",
		    QMessageBox::Yes|QMessageBox::No
		);
		if (reply != QMessageBox::Yes)
			return;
		this->templates.erase(name);
	}

	this->templates.emplace(name, this->getTemplate());
	this->fillTemplates(this->templates);

	QMessageBox::information(this, "Ok", "Šablona " + name + " uložena.", QMessageBox::Ok);
}

void FormSignalEdit::fillTemplates(const TmplStorage &templates) {
	ui.cb_temp_load->clear();
	ui.cb_temp_load->setEnabled(!templates.empty());
	ui.b_temp_load->setEnabled(!templates.empty());
	for (const auto &item : templates)
		ui.cb_temp_load->addItem(item.first);
	if (templates.empty())
		ui.cb_temp_load->addItem("Zatím žádné šablony.");
	else
		ui.cb_temp_load->setCurrentIndex(-1);
}

void FormSignalEdit::tw_outputs_selection_changed() {
	ui.b_delete_signal->setEnabled(!ui.tw_outputs->selectedItems().empty());
	if (ui.tw_outputs->selectedItems().empty()) {
		ui.cb_add_sig->setCurrentIndex(-1);
		ui.sb_add_bits->setValue(0);
	} else {
		ui.cb_add_sig->setCurrentIndex(ui.tw_outputs->selectedItems()[0]->text(0).toInt());
		ui.sb_add_bits->setValue(ui.tw_outputs->selectedItems()[0]->text(2).toInt(nullptr, 2));
	}
}

void FormSignalEdit::sb_output_count_changed(int value) {
	ui.sb_add_bits->setMaximum(pow(2, value)-1);

	for (int i = 0; i < ui.tw_outputs->topLevelItemCount(); ++i) {
		QTreeWidgetItem &item = *(ui.tw_outputs->topLevelItem(i));
		item.setText(2, item.text(2).right(value).rightJustified(value, '0'));
	}
}

void FormSignalEdit::sb_hjop_rcs_add_changed(int value) {
	static int lastValue = 0;
	if (ui.sb_output_addr->value() == lastValue)
		ui.sb_output_addr->setValue(value);
	lastValue = value;
}

} // namespace SignalEdit
