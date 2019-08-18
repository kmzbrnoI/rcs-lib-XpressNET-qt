#ifndef FORMSIGNALEDIT_H
#define FORMSIGNALEDIT_H

#include <QMainWindow>
#include "ui_signal-edit.h"

class FormSignalEdit : public QDialog {
	Q_OBJECT

public:
	FormSignalEdit(QWidget *parent = nullptr);

private:
	Ui::f_signal_edit ui;
};

#endif // FORMSIGNALEDIT_H
