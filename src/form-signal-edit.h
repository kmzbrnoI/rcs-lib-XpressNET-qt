#ifndef FORMSIGNALEDIT_H
#define FORMSIGNALEDIT_H

#include <functional>
#include <QMainWindow>
#include "ui_signal-edit.h"
#include "signals.h"

namespace SignalEdit {

using EditCallback = std::function<void(RcsXn::XnSignal signal)>;

class FormSignalEdit : public QDialog {
	Q_OBJECT

public:
	FormSignalEdit(QWidget *parent = nullptr);
	void open(EditCallback callback);
	void open(RcsXn::XnSignal signal, EditCallback callback);

private slots:
	void b_apply_handle();
	void b_storno_handle();
	void b_temp_load_handle();
	void b_delete_signal_handle();
	void b_add_signal_handle();
	void b_temp_save_handle();

private:
	Ui::f_signal_edit ui;
	EditCallback callback;
};

} // namespace SignalEdit

#endif // FORMSIGNALEDIT_H
