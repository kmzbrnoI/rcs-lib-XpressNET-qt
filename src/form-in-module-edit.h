#ifndef FORM_IN_MODULE_EDIT_H
#define FORM_IN_MODULE_EDIT_H

#include "ui_input-module-edit.h"
#include <QMainWindow>
#include "rcsinputmodule.h"

using InModuleEditCallback = std::function<void(RcsXn::RcsInputModule*)>;

class FormInModuleEdit : public QDialog {
	Q_OBJECT

public:
	RcsXn::RcsInputModule* module = nullptr;

	FormInModuleEdit(QWidget *parent = nullptr);
	void moduleOpen(RcsXn::RcsInputModule* module);
	void setActiveEnabled(bool enabled);

private slots:
	void b_united_time_set_handle();

private:
	Ui::f_module_edit ui;

	void accept() override;
};

#endif // FORM_IN_MODULE_EDIT_H
