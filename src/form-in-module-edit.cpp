#include "form-in-module-edit.h"
#include <QMessageBox>

FormInModuleEdit::FormInModuleEdit(QWidget *parent)
	: QDialog(parent) {
	ui.setupUi(this);
	this->setFixedSize(this->size());
}

void FormInModuleEdit::accept() {
	if (this->module == nullptr)
		return;
	if (this->ui.le_name->text() == "") {
		QMessageBox::warning(this, "Nelze uložit", "Vyplňte název modulu!");
		return;
	}

	this->module->wantActive = this->ui.chb_active->isChecked();
	this->module->name = this->ui.le_name->text();
	this->module->inputFallDelays[0] = static_cast<unsigned>(this->ui.dsb_ind1->value()*10);
	this->module->inputFallDelays[1] = static_cast<unsigned>(this->ui.dsb_ind2->value()*10);
	this->module->inputFallDelays[2] = static_cast<unsigned>(this->ui.dsb_ind3->value()*10);
	this->module->inputFallDelays[3] = static_cast<unsigned>(this->ui.dsb_ind4->value()*10);
	this->module->inputFallDelays[4] = static_cast<unsigned>(this->ui.dsb_ind5->value()*10);
	this->module->inputFallDelays[5] = static_cast<unsigned>(this->ui.dsb_ind6->value()*10);
	this->module->inputFallDelays[6] = static_cast<unsigned>(this->ui.dsb_ind7->value()*10);
	this->module->inputFallDelays[7] = static_cast<unsigned>(this->ui.dsb_ind8->value()*10);

	this->close();
	emit this->accepted();
}

void FormInModuleEdit::moduleOpen(RcsXn::RcsInputModule* module) {
	this->module = module;

	this->ui.chb_active->setChecked(module->wantActive);
	this->ui.le_name->setText(module->name);
	this->ui.dsb_ind1->setValue(static_cast<double>(module->inputFallDelays[0])/10);
	this->ui.dsb_ind2->setValue(static_cast<double>(module->inputFallDelays[1])/10);
	this->ui.dsb_ind3->setValue(static_cast<double>(module->inputFallDelays[2])/10);
	this->ui.dsb_ind4->setValue(static_cast<double>(module->inputFallDelays[3])/10);
	this->ui.dsb_ind5->setValue(static_cast<double>(module->inputFallDelays[4])/10);
	this->ui.dsb_ind6->setValue(static_cast<double>(module->inputFallDelays[5])/10);
	this->ui.dsb_ind7->setValue(static_cast<double>(module->inputFallDelays[6])/10);
	this->ui.dsb_ind8->setValue(static_cast<double>(module->inputFallDelays[7])/10);

	this->setWindowTitle("Vstupní modul " + QString::number(module->addr));
	this->ui.le_name->setFocus();
	this->show();
}

void FormInModuleEdit::setActiveEnabled(bool enabled) {
	this->ui.chb_active->setEnabled(enabled);
}
