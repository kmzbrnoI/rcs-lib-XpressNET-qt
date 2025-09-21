#include "form-in-module-edit.h"

FormInModuleEdit::FormInModuleEdit() {
}

void FormInModuleEdit::accept() {
    this->close();
    emit this->accepted();
}
