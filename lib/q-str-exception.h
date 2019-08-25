#ifndef RCS_XN_Q_STR_EXCEPTION_H
#define RCS_XN_Q_STR_EXCEPTION_H

#include <QString>

namespace RcsXn {

class QStrException {
private:
	QString m_err_msg;

public:
	QStrException(const QString &msg) : m_err_msg(msg) {}
	~QStrException() noexcept = default;
	QString str() const noexcept { return this->m_err_msg; }
	operator QString() const { return this->m_err_msg; }
};

} // namespace RcsXn

#endif
