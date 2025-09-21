#ifndef RCS_XN_H
#define RCS_XN_H

/* This is the main file of RCS-XN library. It defines RcsXn class which holds
 * all the library's data.
 */

#include <QCoreApplication>
#include <QMainWindow>
#include <QThread>
#include <QtCore/QtGlobal>
#include <QtGlobal>
#include <QTimer>
#include <array>
#include <map>
#include <queue>

#include "common.h"
#include "events.h"
#include "form-signal-edit.h"
#include "lib/q-str-exception.h"
#include "lib/xn-lib-cpp-qt/xn.h"
#include "settings.h"
#include "signals.h"
#include "ui_main-window.h"
#include "rcsinputmodule.h"

namespace RcsXn {

enum class RcsXnLogLevel {
	llNo = 0,
	llError = 1,
	llWarning = 2,
	llInfo = 3,
	llCommands = 4,
	llRawCommands = 5,
	llDebug = 6,
};

enum class RcsStartState {
	stopped = 0,
	scanning = 1,
	started = 2,
};

struct EInvalidRange : public QStrException {
	EInvalidRange(const QString &str) : QStrException(str) {}
};

///////////////////////////////////////////////////////////////////////////////

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	Ui::MainWindow ui;
	MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) { ui.setupUi(this); }
};

///////////////////////////////////////////////////////////////////////////////

struct AccReset {
	AccReset(unsigned int id, unsigned int portAddr, QDateTime resetTime)
		: id(id), portAddr(portAddr), resetTime(resetTime) {}

	unsigned int id;
	unsigned int portAddr;
	QDateTime resetTime;
};

///////////////////////////////////////////////////////////////////////////////

class RcsXn : public QObject {
	Q_OBJECT

public:
	RcsEvents events;
	Xn::XpressNet xn;
	Settings s;
	RcsXnLogLevel loglevel = RcsXnLogLevel::llInfo;
	RcsStartState started = RcsStartState::stopped;
	bool opening = false;
	std::array<RcsInputModule, IO_IN_MODULES_COUNT> modules_in;
	std::array<bool, IO_COUNT> inputs;
	std::array<bool, IO_COUNT> outputs;
	std::array<bool, IO_IN_MODULES_COUNT> real_active_in; // 0-255
	std::array<bool, IO_IN_MODULES_COUNT> user_active_in; // 0-255
	std::array<bool, IO_OUT_MODULES_COUNT> user_active_out; // 0-1023
	std::array<bool, IO_OUT_MODULES_COUNT> binary; // 0-1023
	uint8_t scan_group;
	QString config_filename = "";
	unsigned int li_ver_hw = 0, li_ver_sw = 0;
	unsigned int modules_count = 0;
	unsigned int in_count = 0, out_count = 0;

	// signals
	SigTmplStorage sigTemplates;
	SigStorage sig;

	// UI
	MainWindow form;
	SignalEdit::FormSignalEdit f_signal_edit;
	bool gui_config_changing = false;

	explicit RcsXn(QObject *parent = nullptr);
	~RcsXn() override;

	void log(const QString &msg, RcsXnLogLevel loglevel);
	void error(const QString &message, uint16_t code, unsigned int module);
	void error(const QString &message, uint16_t code);
	void error(const QString &message);
	void first_scan();
	void setLogLevel(RcsXnLogLevel);

	int openDevice(const QString &device, bool persist);
	int close();
	void loadConfig(const QString &filename);
	void saveConfig(const QString &filename);
	int start();
	int stop();

	int setPlainOutput(unsigned int portAddr, int state, bool setInternalState = true);
	void xnSetOutputOk(unsigned int portAddr, int state);
	void xnSetOutputError(unsigned int module);

	bool isSignal(unsigned int portAddr) const; // 0-2047
	int setSignal(unsigned int portAddr, unsigned int code); // returns same error codes as SetOutput
	bool isResettingSignals() const;

private slots:
	void xnOnError(QString error);
	void xnOnLog(QString message, Xn::LogLevel loglevel);
	void xnOnConnect();
	void xnOnDisconnect();
	void xnOnTrkStatusChanged(Xn::TrkStatus);
	void xnOnAccInputChanged(uint8_t groupAddr, bool nibble, bool error, Xn::FeedbackType inputType,
	                         Xn::AccInputsState state);

	void resetNextSignal();

	void m_acc_reset_timer_tick();

	// GUI
	void cb_loglevel_changed(int);
	void cb_interface_type_changed(int);
	void cb_connections_changed(int);
	void b_serial_refresh_handle();
	void b_active_load_handle();
	void b_active_save_handle();
	void tw_log_double_clicked(QTreeWidgetItem *item, int column);
	void b_signal_add_handle();
	void b_signal_remove_handle();
	void tw_signals_dbl_click(QTreeWidgetItem *, int);
	void tw_signals_selection_changed();
	void chb_general_config_changed(int state);
	void b_dcc_on_handle();
	void b_dcc_off_handle();
	void b_apply_binary_handle();

private:
	unsigned int m_acc_op_pending_count = 0;
	QTimer m_acc_reset_timer;
	std::deque<AccReset> m_accToResetDeq;
	std::array<unsigned int, IO_COUNT> m_accToResetArr;
	QTimer m_resetSignalsTimer;
	SigStorage::iterator m_resetSignalsIt;

	void xnGotLIVersion(void *, unsigned hw, unsigned sw);
	void xnOnLIVersionError(void *, void *);
	void xnOnCSStatusError(void *, void *);
	void xnOnInitScanningError(void *, void *);
	void xn_onDccError(void *, void *);
	void xn_onDccOpenError(void *, void *);
	void initModuleScanned(uint8_t group, bool nibble);
	void scanNextGroup(int previousGroup);
	void initScanningDone();
	Xn::LIType interface(const QString &name) const;
	uint8_t inBusModuleAddr(uint8_t userAddr);

	template <std::size_t ArraySize>
	void parseModules(const QString &active, std::array<bool, ArraySize> &result,
	                  bool except = true);

	template <std::size_t ArraySize>
	QString getActiveStr(const std::array<bool, ArraySize> &source, const QString &separator);

	void loadActiveIO(const QString &inputs, const QString &outputs, bool except = true);
	void resetIOState();

	void loadSignals(QSettings &s);
	void saveSignals(QSettings &s) const;

	void loadInputModules(QSettings &s);
	void saveInputModules(QSettings &s) const;

	unsigned int current_editing_signal;
	void newSignal(XnSignal);
	void editedSignal(XnSignal);
	void resetSignals();

	void setDcc(Xn::TrkStatus);
	void widgetSetColor(QWidget &widget, const QColor &color);

	// GUI:
	void guiInit();
	void fillConnectionsCbs();
	void fillPortCb();
	void guiOnOpen();
	void guiOnClose();
	void fillActiveIO();
	void fillSignals();
	void guiAddSignal(const XnSignal &);
	void fillInputModules();
};

///////////////////////////////////////////////////////////////////////////////
// Templated method must be in header file

template <std::size_t ArraySize>
QString RcsXn::getActiveStr(const std::array<bool, ArraySize> &source, const QString &separator) {
	QString output;
	for (size_t start = 0; start < source.size(); ++start) {
		if (source[start]) {
			size_t end = start;
			while (source[end] && end < source.size())
				++end;
			if (end == start+1)
				output += QString::number(start)+separator;
			else
				output += QString::number(start)+"-"+QString::number(end-1)+separator;
			start = end;
		}
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////////

// Dirty magic for Qt's event loop
// This class should be created first
struct AppThread {
	AppThread() {
		if (qApp == nullptr) {
			int argc = 0;
			QApplication* app = new QApplication(argc, nullptr);
			QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
			app->exec();
		}
	}

};

extern AppThread main_thread;
extern RcsXn rx;

} // namespace RcsXn

#endif
