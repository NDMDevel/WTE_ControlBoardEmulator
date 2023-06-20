#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
//----------------------------------------------
private slots:
    void on_serialConnect();
    void on_rx();
    void on_updatePortList();
    void on_clearLog();
    void on_clearHistory();
    void on_autoreadTimeout();
private:
    void showMessage(QString title,QString msg);
    void configUiOnSerialConnection(bool serialConnected);
    void processCommand(QString command);
    void closeEvent(QCloseEvent *event) override;
    QString getByteProcessed(QString str,bool printByteDetails);
    void logToRxConsole(QString str);
    bool isTxOnlyLog();
    bool isRxOnlyLog();
    void logAppend(QString str);
    bool printDetails();
    QByteArray read_io_all();
private:
    static constexpr uint32_t _defaultBaudRate = 460800;
private:
    QSerialPort _com;
    QString     _rxQueue;
    enum class parser_conf_t : uint8_t
    {
        quiet,
        verbose,
        autoread
    }_parser_conf = parser_conf_t::verbose;
    QTimer _timer;
};
#endif // MAINWINDOW_H
