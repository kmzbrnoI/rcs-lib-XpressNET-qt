#ifndef FORMSIGNALEDIT_H
#define FORMSIGNALEDIT_H

#include "signals.h"
#include "ui_signal-edit.h"
#include <QMainWindow>
#include <functional>

namespace SignalEdit {

using EditCallback = std::function<void(RcsXn::XnSignal signal)>;
using TmplStorage = std::map<QString, RcsXn::XnSignalTemplate>;

class FormSignalEdit : public QDialog {
	Q_OBJECT

public:
	FormSignalEdit(TmplStorage &templates, QWidget *parent = nullptr);
	void open(EditCallback callback, TmplStorage &templates);
	void open(RcsXn::XnSignal signal, EditCallback callback, TmplStorage &templates);

private slots:
	void b_apply_handle();
	void b_storno_handle();
	void b_temp_load_handle();
	void b_delete_signal_handle();
	void b_add_signal_handle();
	void b_temp_save_handle();
	void tw_outputs_selection_changed();
	void sb_output_count_changed(int);

private:
	Ui::f_signal_edit ui;
	EditCallback callback;
	TmplStorage &templates;

	void fillTemplates(const TmplStorage &);
	void fillTemplate(const RcsXn::XnSignalTemplate &);
	RcsXn::XnSignalTemplate getTemplate() const;
};

} // namespace SignalEdit

#endif // FORMSIGNALEDIT_H
