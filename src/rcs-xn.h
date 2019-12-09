#ifndef RCS_XN_H
#define RCS_XN_H

/* This is the main file of RCS-XN library. It defined exported functions as
 * well as RcsXn class which holds all the library's data.
 */

#include <QCoreApplication>
#include <QMainWindow>
#include <QThread>
#include <QtCore/QtGlobal>
#include <QtGlobal>
#include <array>
#include <map>

#include "form-signal-edit.h"
#include "lib/q-str-exception.h"
#include "lib/xn-lib-cpp-qt/xn.h"
#include "settings.h"
#include "signals.h"
#include "ui_main-window.h"
#include "events.h"

namespace RcsXn {

constexpr size_t IO_COUNT = 2048;
constexpr size_t IO_MODULE_PIN_COUNT = 2;
constexpr size_t IO_MODULES_COUNT = IO_COUNT / IO_MODULE_PIN_COUNT;

const QColor LOGC_ERROR = QColor(0xFF, 0xAA, 0xAA);
const QColor LOGC_WARN = QColor(0xFF, 0xFF, 0xAA);
const QColor LOGC_DONE = QColor(0xAA, 0xFF, 0xAA);

constexpr std::array<unsigned int, 1> API_SUPPORTED_VERSIONS {
    0x0301, // v1.3
};

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

class RcsXn : public QObject {
	Q_OBJECT

public:
	RcsEvents events;
	Xn::XpressNet xn;
	Settings s;
	RcsXnLogLevel loglevel = RcsXnLogLevel::llInfo;
	RcsStartState started = RcsStartState::stopped;
	bool opening = false;
	std::array<bool, IO_COUNT> inputs;
	std::array<bool, IO_COUNT> outputs;
	std::array<bool, IO_MODULES_COUNT> active_in;
	std::array<bool, IO_MODULES_COUNT> active_out;
	uint8_t scan_group;
	bool scan_nibble;
	unsigned int api_version = 0x0301;
	QString config_filename = "";
	unsigned int li_ver_hw = 0, li_ver_sw = 0;
	unsigned int modules_count = 0;
	unsigned int in_count = 0, out_count = 0;

	// UI
	MainWindow form;
	SignalEdit::FormSignalEdit f_signal_edit;
	bool gui_config_changing = false;

	// signals
	SigTmplStorage sigTemplates;
	SigStorage sig;

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

	int setPlainOutput(unsigned int portAddr, int state);
	void xnSetOutputError(void *sender, void *data);

	bool isSignal(unsigned int portAddr) const; // 0-2047
	void setSignal(unsigned int portAddr, unsigned int code);

private slots:
	void xnOnError(QString error);
	void xnOnLog(QString message, Xn::LogLevel loglevel);
	void xnOnConnect();
	void xnOnDisconnect();
	void xnOnTrkStatusChanged(Xn::TrkStatus);
	void xnOnAccInputChanged(uint8_t groupAddr, bool nibble, bool error, Xn::FeedbackType inputType,
	                         Xn::AccInputsState state);

	// GUI
	void cb_loglevel_changed(int);
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

private:
	void xnGotLIVersion(void *, unsigned hw, unsigned sw);
	void xnOnLIVersionError(void *, void *);
	void xnOnCSStatusError(void *, void *);
	void xnOnInitScanningError(void *, void *);
	void xn_onDccError(void *, void *);
	void xn_onDccOpenError(void *, void *);
	void initModuleScanned(uint8_t group, bool nibble);
	void initScanningDone();
	Xn::LIType interface(QString name);

	template <std::size_t ArraySize>
	void parseActiveModules(const QString &active, std::array<bool, ArraySize> &result,
	                        bool except = true);

	template <std::size_t ArraySize>
	QString getActiveStr(const std::array<bool, ArraySize> &source, const QString &separator);

	void loadActiveIO(const QString &inputs, const QString &outputs, bool except = true);
	void resetIOState();

	void loadSignals(const QString &filename);
	void saveSignals(const QString &filename);

	unsigned int current_editing_signal;
	void newSignal(XnSignal);
	void editedSignal(XnSignal);

	void setDcc(Xn::TrkStatus);
	void widgetSetColor(QWidget &widget, const QColor &color);

	// GUI:
	void fillConnectionsCbs();
	void fillPortCb();
	void guiOnOpen();
	void guiOnClose();
	void fillActiveIO();
	void fillSignals();
	void guiAddSignal(const XnSignal &);
};

///////////////////////////////////////////////////////////////////////////////

// Dirty magic for Qt's event loop
// This class should be created first
class AppThread {
	std::unique_ptr<QApplication> app;
	int argc {0};

public:
	AppThread() {
		app = std::make_unique<QApplication>(argc, nullptr);
		QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
		app->exec();
	}
};

extern AppThread main_thread;
extern RcsXn rx;

} // namespace RcsXn

#endif
