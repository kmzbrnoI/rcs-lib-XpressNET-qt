#include <QMessageBox>
#include "form-signal-edit.h"
#include "lib/q-str-exception.h"

namespace SignalEdit {

FormSignalEdit::FormSignalEdit(TmplStorage &templates, QWidget *parent)
	: QDialog(parent), templates(templates) {
	ui.setupUi(this);

	for (size_t i = 0; i < RcsXn::XnSignalCodes.size(); ++i)
		ui.cb_add_sig->addItem(QString::number(i) + ": " + RcsXn::XnSignalCodes[i]);

	QObject::connect(ui.b_apply, SIGNAL(released()), this, SLOT(b_apply_handle()));
	QObject::connect(ui.b_storno, SIGNAL(released()), this, SLOT(b_storno_handle()));
	QObject::connect(ui.b_temp_load, SIGNAL(released()), this, SLOT(b_temp_load_handle()));
	QObject::connect(ui.b_delete_signal, SIGNAL(released()), this, SLOT(b_delete_signal_handle()));
	QObject::connect(ui.b_add_signal, SIGNAL(released()), this, SLOT(b_add_signal_handle()));
	QObject::connect(ui.b_temp_save, SIGNAL(released()), this, SLOT(b_temp_save	_handle()));
}

void FormSignalEdit::open(EditCallback callback, TmplStorage &templates) {
	this->callback = callback;
	this->templates = templates;
}

void FormSignalEdit::open(RcsXn::XnSignal signal, EditCallback callback, TmplStorage &templates) {
	this->callback = callback;
	this->templates = templates;

	ui.le_name->setText(signal.name);
	ui.sb_hjop_rcs_addr->setValue(signal.hJOPaddr);
	ui.sb_output_addr->setValue(signal.startAddr);
	ui.sb_output_count->setValue(signal.tmpl.outputsCount);

	ui.tv_outputs->clear();
	for (const auto &output : signal.tmpl.outputs) {
		auto *item = new QTreeWidgetItem(ui.tv_outputs);
		item->setText(1, QString::number(output.first));
		if (output.first < RcsXn::XnSignalCodes.size())
			item->setText(1, RcsXn::XnSignalCodes[output.first]);
		else
			item->setText(1, "?");
		item->setText(2, QString::number(output.second, 2));
		ui.tv_outputs->addTopLevelItem(item);
	}

	ui.sb_add_bits->setValue(0);

	this->show();
}

void FormSignalEdit::b_apply_handle() {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();

	RcsXn::XnSignal result;

	result.name = ui.le_name->text();
	result.hJOPaddr = ui.sb_hjop_rcs_addr->value();
	result.startAddr = ui.sb_output_addr->value();
	result.tmpl.outputsCount = ui.sb_output_count->value();

	for (int i = 0; i < ui.tv_outputs->topLevelItemCount(); ++i) {
		const QTreeWidgetItem *item = ui.tv_outputs->topLevelItem(i);
		result.tmpl.outputs.emplace(item->text(0).toUInt(), item->text(2).toUInt(nullptr, 2));
	}

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
}

void FormSignalEdit::b_delete_signal_handle() {
}

void FormSignalEdit::b_add_signal_handle() {
}

void FormSignalEdit::b_temp_save_handle() {
}

} // namespace SignalEdit
