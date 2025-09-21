#ifndef FORM_IN_MODULE_EDIT_H
#define FORM_IN_MODULE_EDIT_H

#include "ui_input-module-edit.h"
#include <QMainWindow>

class FormInModuleEdit : public QDialog {
	Q_OBJECT

public:
	FormInModuleEdit();

private slots:

private:
	Ui::f_module_edit ui;

	void accept() override;

};

#endif // FORM_IN_MODULE_EDIT_H
