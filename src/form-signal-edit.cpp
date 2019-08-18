#include "form-signal-edit.h"

namespace SignalEdit {

FormSignalEdit::FormSignalEdit(QWidget *parent)
	: QDialog(parent) {
	ui.setupUi(this);

	QObject::connect(ui.b_apply, SIGNAL(released()), this, SLOT(b_apply_handle()));
	QObject::connect(ui.b_storno, SIGNAL(released()), this, SLOT(b_storno_handle()));
	QObject::connect(ui.b_temp_load, SIGNAL(released()), this, SLOT(b_temp_load_handle()));
	QObject::connect(ui.b_delete_signal, SIGNAL(released()), this, SLOT(b_delete_signal_handle()));
	QObject::connect(ui.b_add_signal, SIGNAL(released()), this, SLOT(b_add_signal_handle()));
	QObject::connect(ui.b_temp_save, SIGNAL(released()), this, SLOT(b_temp_save	_handle()));
}

void FormSignalEdit::open(EditCallback callback) {
	this->callback = callback;
}

void FormSignalEdit::open(RcsXn::XnSignal signal, EditCallback callback) {
	this->callback = callback;
}

void FormSignalEdit::b_apply_handle() {
}

void FormSignalEdit::b_storno_handle() {
}

void FormSignalEdit::b_temp_load_handle() {
}

void FormSignalEdit::b_delete_signal_handle() {
}

void FormSignalEditb_add_signal_handle() {
}

void FormSignalEdit::b_temp_save_handle() {
}

} // namespace SignalEdit
