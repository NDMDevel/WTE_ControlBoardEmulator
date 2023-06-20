#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSerialPortInfo>
#include <QMessageBox>
#include "control_io.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    on_updatePortList();

    //btn connect
    connect(ui->btn_connect,SIGNAL(clicked()),
            this,SLOT(on_serialConnect()));
    //serial rx
    connect(&_com,SIGNAL(readyRead()),
            this,SLOT(on_rx()));
    //clear log & history
    connect(ui->btn_clearLog,SIGNAL(clicked()),
            this,SLOT(on_clearLog()));
    connect(ui->bnt_clearHistory,SIGNAL(clicked()),
            this,SLOT(on_clearHistory()));
    //autoread T
    connect(&_timer,SIGNAL(timeout()),
            this,SLOT(on_autoreadTimeout()));
}
MainWindow::~MainWindow()
{
    delete ui;
}

//slots
void MainWindow::on_serialConnect()
{
    if( !_com.isOpen() )
    {
        QString port  = ui->combo_serialPort->currentText();//ui->port_address->text();
        if( port.contains(':') )
            port = port.left(port.indexOf(':'));
        bool ok;
        uint32_t baud = ui->edit_baudrate->text().toUInt(&ok);
        if( !ok )
        {
            auto strBaud = QString::number(_defaultBaudRate);
            showMessage("Error","Invalid baudrate, defaulting to " + strBaud);
            ui->edit_baudrate->setText(strBaud);
            return;
        }
        _com.setPortName(port);
        _com.setBaudRate(baud);
        _com.setDataBits(QSerialPort::DataBits::Data8);
        _com.setStopBits(QSerialPort::StopBits::OneStop);
        _com.setParity(QSerialPort::Parity::NoParity);
        if( _com.open(QIODevice::ReadWrite) )
            configUiOnSerialConnection(true);
        else
            showMessage("Error",QString("Connection to serial port '%1' failed (may be already in use)").arg(port));
    }
    else
    {
        _com.close();
        configUiOnSerialConnection(false);
    }
}
void MainWindow::on_rx()
{
    auto array = _com.read(_com.bytesAvailable());
    _rxQueue += array;
    logToRxConsole(array);
    if( auto idx=_rxQueue.indexOf('\r'); idx != -1 )
    {
        QString command = _rxQueue.left(idx+1);
        _rxQueue.remove(0,idx+1);

        if( ui->combo_logOption->currentIndex() != 1 )
        {
            auto rx = "RX: " + getByteProcessed(command,ui->check_details->isChecked());
            ui->console_log->appendPlainText(rx);
        }

        //removes '\r'
        command.resize(command.size()-1);
        processCommand(command);
    }
}
void MainWindow::on_updatePortList()
{
    ui->combo_serialPort->clear();
    auto ports = QSerialPortInfo::availablePorts();
    for( const auto& port : ports )
        ui->combo_serialPort->addItem(port.portName() + ": " + port.description());
}
void MainWindow::on_clearLog()
{
    ui->console_log->clear();
}
void MainWindow::on_clearHistory()
{
    ui->console_rx->clear();
}
void MainWindow::on_autoreadTimeout()
{
    QString array = read_io_all();
    array += '\n';
    _com.write(array.toUtf8());
    if( !isRxOnlyLog() )
        logAppend("TX: "+getByteProcessed( array , printDetails() ));
}

//general methods
void MainWindow::showMessage(QString title, QString msg)
{
    QMessageBox box;
    box.setWindowTitle(title);
    box.setText(msg);
    box.exec();
}
void MainWindow::configUiOnSerialConnection(bool serialConnected)
{
    if( serialConnected )
        ui->btn_connect->setText("Disconnect");
    else
        ui->btn_connect->setText("Connect");
    ui->combo_serialPort->setEnabled(!serialConnected);
    ui->edit_baudrate->setEnabled(!serialConnected);
}
void MainWindow::processCommand(QString command)
{
    using namespace wte;
    auto tokens = command.split(' ');
    if( tokens.length() == 0 )
        return;

    if( tokens[0] == "write" )
    {
        if( tokens.length() != 3 )
            return;

        //helper lambdas
        //[out] dio/do
        const auto toInt    = [](digital_out d)->uint8_t{ return static_cast<uint8_t>(d); };
        const auto toDo     = [](uint8_t i)->digital_out{ return static_cast<digital_out>(i);};
        const auto isDo     = [&](QString token)->std::optional<digital_out>
        {
            for( uint8_t i=toInt(digital_out::out0) ; i<=toInt(digital_out::out4) ; i++ )
                if( token == "do"+QString::number(i) )
                    return toDo(i);
            return std::nullopt;
        };
        const auto isDio    = [&](QString token)->std::optional<digital_out>
        {
            for( uint8_t i=toInt(digital_out::io_out0)-toInt(digital_out::io_out0) ; i<=toInt(digital_out::io_out3)-toInt(digital_out::io_out0) ; i++ )
                if( token == "dio"+QString::number(i) )
                    return toDo(i);
            return std::nullopt;
        };
        const auto getDio   = [&](const QString arg,const QString value)->std::optional<std::pair<digital_out,bool>>
        {
            if( (arg.startsWith("dio") && arg.length() == 4) && value.length() != 0 )
                if( auto d=isDio(arg); d.has_value() )
                {
                    bool ok;
                    int val = value.toInt(&ok);
                    if( !ok )
                        return std::nullopt;
                    return {{d.value(),val != 0}};
                }
            return std::nullopt;
        };
        const auto getDo    = [&](const QString arg,const QString value)->std::optional<std::pair<digital_out,bool>>
        {
            if( (arg.startsWith("do") && arg.length() == 3) && value.length() != 0 )
                if( auto d=isDo(arg); d.has_value() )
                {
                    bool ok;
                    int val = value.toInt(&ok);
                    if( !ok )
                        return std::nullopt;
                    return {{d.value(),val != 0}};
                }
            return std::nullopt;
        };
        //[out] an
        const auto isAnOut  = [&](QString token)->std::optional<analog_out>
        {
            if( token == "an0" )
                return analog_out::out0;
            if( token == "an1" )
                return analog_out::out1;
            return std::nullopt;
        };
        const auto getAnOut = [&](const QString arg,const QString value)->std::optional<std::pair<analog_out,float>>
        {
            if( (arg.startsWith("an") && arg.length() == 3) && value.length() != 0 )
                if( auto d=isAnOut(arg); d.has_value() )
                {
                    bool ok;
                    float val = value.toFloat(&ok);
                    if( !ok )
                        return std::nullopt;
                    return {{d.value(),val}};
                }
            return std::nullopt;
        };
        const auto anToInt  = [](analog_out a)->uint8_t{ return static_cast<uint8_t>(a); };

        //write dio
        if( auto dio=getDio(tokens[1],tokens[2]); dio.has_value() )
        {
            auto[ch,value] = dio.value();
            write_digital_out(ch,value);
            if( _parser_conf == parser_conf_t::verbose )
            {
                auto ans = QString(" > write dio%1 %2\n").arg(toInt(ch)).arg(int(value));
                if( !isRxOnlyLog() )
                    logAppend("TX: "+getByteProcessed( ans , printDetails() ));
                _com.write(ans.toUtf8());
            }
            return;
        }
        //write doX
        if( auto _do=getDo(tokens[1],tokens[2]); _do.has_value() )
        {
            auto[ch,value] = _do.value();
            write_digital_out(ch,value);
            if( _parser_conf == parser_conf_t::verbose )
            {
                auto ans = QString(" > write do%1 %2\n").arg(toInt(ch)).arg(int(value));
                if( !isRxOnlyLog() )
                    logAppend("TX: "+getByteProcessed( ans , printDetails() ));
                _com.write(ans.toUtf8());
            }
            return;
        }
        //write anX
        if( auto an=getAnOut(tokens[1],tokens[2]); an.has_value() )
        {
            auto[ch,value] = an.value();
            write_analog_out(ch,value);
            if( _parser_conf == parser_conf_t::verbose )
            {
                auto ans = QString(" > write an%1 %2\n").arg(anToInt(ch)).arg(value);
                if( !isRxOnlyLog() )
                    logAppend("TX: "+getByteProcessed( ans , printDetails() ));
                _com.write(ans.toUtf8());
            }
        }
        return;
    }
    if( tokens[0] == "read" )
    {
        if( tokens.length() != 2 )
            return;

        //read dio
        if( tokens[1].startsWith("dio") && tokens[1].length() == 4 )
        {
            bool ok;
            int ch = tokens[1].remove(0,3).toInt(&ok);
            if( !ok || ch<0 || ch>3 )
                return;
            bool value = read_digital_in(static_cast<digital_in>(ch+static_cast<uint8_t>(digital_in::io_in0)));
            if( _parser_conf == parser_conf_t::verbose )
            {
                auto ans = QString(" > read dio%1 %2\n").arg(ch).arg(value);
                if( !isRxOnlyLog() )
                    logAppend("TX: "+getByteProcessed( ans , printDetails() ));
                _com.write(ans.toUtf8());
            }
            if( _parser_conf == parser_conf_t::quiet )
            {
                auto ans = QString("%1\n").arg(value);
                if( !isRxOnlyLog() )
                    logAppend("TX: "+getByteProcessed( ans , printDetails() ));
                _com.write(ans.toUtf8());
            }
            return;
        }
        //read diX
        if( tokens[1].startsWith("di") && tokens[1].length() == 3 )
        {
            bool ok;
            int ch = tokens[1].remove(0,2).toInt(&ok);
            if( !ok || ch<0 || ch>3 )
                return;
            bool value = read_digital_in(static_cast<digital_in>(ch));
            if( _parser_conf == parser_conf_t::verbose )
            {
                auto ans = QString(" > read dio%1 %2\n").arg(ch).arg(value);
                if( !isRxOnlyLog() )
                    logAppend("TX: "+getByteProcessed( ans , printDetails() ));
                _com.write(ans.toUtf8());
            }
            if( _parser_conf == parser_conf_t::quiet )
            {
                auto ans = QString("%1\n").arg(value);
                if( !isRxOnlyLog() )
                    logAppend("TX: "+getByteProcessed( ans , printDetails() ));
                _com.write(ans.toUtf8());
            }
            return;
        }
        //read anX
        if( tokens[1].startsWith("an") && tokens[1].length() == 3 )
        {
            bool ok;
            int ch = tokens[1].remove(0,2).toInt(&ok);
            if( !ok || ch<0 || ch>1 )
                return;
            float value = read_analog_in(static_cast<analog_in>(ch));
            if( _parser_conf == parser_conf_t::verbose )
            {
                auto ans = QString(" > read an%1 %2\n").arg(ch).arg(value);
                if( !isRxOnlyLog() )
                    logAppend("TX: "+getByteProcessed( ans , printDetails() ));
                _com.write(ans.toUtf8());
            }
            if( _parser_conf == parser_conf_t::quiet )
            {
                auto ans = QString("%1\n").arg(value);
                if( !isRxOnlyLog() )
                    logAppend("TX: "+getByteProcessed( ans , printDetails() ));
                _com.write(ans.toUtf8());
            }
            return;
        }
        //read all
        if( tokens[1] == "all" )
        {
            QString array = read_io_all();
            array += "\n";
            if( _parser_conf == parser_conf_t::verbose )
            {
                auto ans = QString(" > read all ") + array;
                if( !isRxOnlyLog() )
                    logAppend("TX: "+getByteProcessed( ans , printDetails() ));
                _com.write(ans.toUtf8());
                return;
            }
            if( _parser_conf == parser_conf_t::quiet )
            {
                if( !isRxOnlyLog() )
                    logAppend("TX: "+getByteProcessed( array , printDetails() ));
                _com.write(array.toUtf8());
                return;
            }
        }
        return;
    }
    if( tokens[0] == "reset" )
    {
        if( tokens.length() != 1 )
            return;
        _timer.stop();
        write_analog_out(analog_out::out0,0.0f);
        write_analog_out(analog_out::out1,0.0f);
        write_digital_out(digital_out::out0,false);
        write_digital_out(digital_out::out1,false);
        write_digital_out(digital_out::out2,false);
        write_digital_out(digital_out::out3,false);
        write_digital_out(digital_out::out4,false);
        write_digital_out(digital_out::io_out0,false);
        write_digital_out(digital_out::io_out1,false);
        write_digital_out(digital_out::io_out2,false);
        write_digital_out(digital_out::io_out3,false);
        _parser_conf = parser_conf_t::verbose;
        if( _parser_conf == parser_conf_t::verbose )
        {
            auto ans = QString(" > reset\n");
            if( !isRxOnlyLog() )
                logAppend("TX: "+getByteProcessed( ans , printDetails() ));
            _com.write(ans.toUtf8());
        }
        return;
    }
    if( tokens[0] == "help" )
    {
        if( tokens.length() != 1 )
            return;

        const char help[] =R"=====( > help:
'read dioX'    with X=[0:3]
'read diX'     with X=[0:3]
'read anX'     with X=[0:1]
'read all'     in_an01 out_an01 di_dioI_do_dioO
'write dioX V' with X=[0:3],V=[0:1]
'write doX V'  with X=[0:4],V=[0:1]
'write anX V'  with X=[0:1],V=[0.0:100.0]
'reset'        write zero to all analog an digital
'conf X'       with X=[verbose|quiet|autoread T],T=[0:999999]
               T is the acquisition period (in ms)
'help'         print this)=====";

        auto ans = QString(help) + "\n";
        if( !isRxOnlyLog() )
            logAppend("TX: "+getByteProcessed( ans , printDetails() ));
        _com.write(ans.toUtf8());
        return;
    }
    if( tokens[0] == "conf" )
    {
        if( tokens.length() != 2 && tokens.length() != 3 )
            return;
        if( tokens[1] == "verbose" && tokens.length() == 2 )
        {
            _timer.stop();
            _parser_conf = parser_conf_t::verbose;
            auto ans = QString(" > conf verbose\n");
            if( !isRxOnlyLog() )
                logAppend("TX: "+getByteProcessed( ans , printDetails() ));
            _com.write(ans.toUtf8());
            return;
        }
        if( tokens[1] == "quiet" && tokens.length() == 2 )
        {
            _timer.stop();
            _parser_conf = parser_conf_t::quiet;
            return;
        }
        if( tokens[1] == "autoread" && tokens.length() == 3 )
        {
            bool ok;
            int ms = tokens[2].toInt(&ok);
            if( !ok || ms<0 || ms>999999 )
                return;
            _timer.setInterval(ms);
            _timer.start();
            _parser_conf = parser_conf_t::autoread;
            return;
        }
    }
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    if( _com.isOpen() )
        _com.close();
    event->accept();
}
QString MainWindow::getByteProcessed(QString str,bool printByteDetails)
{
    QString rx = str.chopped(1); //removes last char (\r or \n)
    if( printByteDetails )
    {
        rx += "  (byte details:";
        for( auto b : str.toUtf8() )
        {
            auto dec = uint8_t(b);
            rx += " ";
            if( b == '\r' )
                rx += "("+QString::number(dec)+"):" + "'\\r'";
            else if( b == '\t' )
                rx += "("+QString::number(dec)+"):" + "'\\t'";
            else if( b == '\n' )
                rx += "("+QString::number(dec)+"):" + "'\\n'";
            else
                rx += "("+QString::number(dec)+"):" + "'" + b + "'";
        }
        rx += ")";
    }
    return rx;
}
void MainWindow::logToRxConsole(QString str)
{
    for( auto b : str.toUtf8() )
        if( b == '\r' )
            ui->console_rx->appendPlainText(QString::number(uint8_t(b))+": '\\r'");
        else if( b == '\n' )
            ui->console_rx->appendPlainText(QString::number(uint8_t(b))+": '\\n'");
        else if( b == '\t' )
            ui->console_rx->appendPlainText(QString::number(uint8_t(b))+": '\\t'");
        else
            ui->console_rx->appendPlainText(QString::number(uint8_t(b))+": '"+b+"'");
}
bool MainWindow::isTxOnlyLog()
{
    return ui->combo_logOption->currentIndex() == 1;
}
bool MainWindow::isRxOnlyLog()
{
    return ui->combo_logOption->currentIndex() == 2;
}
void MainWindow::logAppend(QString str)
{
    ui->console_log->appendPlainText(str);
}
bool MainWindow::printDetails()
{
    return ui->check_details->isChecked();
}
QByteArray MainWindow::read_io_all()
{
    using namespace wte;
    char str[64];
    sprintf(str,"%.2f %.2f %.2f %.2f ",
            read_analog_in(analog_in::in0),
            read_analog_in(analog_in::in1),
            read_analog_out(analog_out::out0),
            read_analog_out(analog_out::out1));
    uint8_t di  = rand() % 15 + 64;
    uint8_t dio = rand() % 15 + 64;
    static union
    {
        struct
        {
            uint8_t out0 : 1;
            uint8_t out1 : 1;
            uint8_t out2 : 1;
            uint8_t out3 : 1;
            uint8_t out4 : 1;
            uint8_t rsv  : 2;
            uint8_t msb  : 1;
        };
        uint8_t raw;
    }_digital_out;
    _digital_out.out0 = read_digital_out(digital_out::out0);
    _digital_out.out1 = read_digital_out(digital_out::out1);
    _digital_out.out2 = read_digital_out(digital_out::out2);
    _digital_out.out3 = read_digital_out(digital_out::out3);
    _digital_out.out4 = read_digital_out(digital_out::out4);
    _digital_out.rsv = 2;
    static union
    {
        struct
        {
            uint8_t io_out0 : 1;
            uint8_t io_out1 : 1;
            uint8_t io_out2 : 1;
            uint8_t io_out3 : 1;
            uint8_t rsv : 3;
            uint8_t msb : 1;
        };
        uint8_t raw;
    }_digital_io_out;
    _digital_io_out.io_out0 = read_digital_out(digital_out::io_out0);
    _digital_io_out.io_out1 = read_digital_out(digital_out::io_out1);
    _digital_io_out.io_out2 = read_digital_out(digital_out::io_out2);
    _digital_io_out.io_out3 = read_digital_out(digital_out::io_out3);
    _digital_io_out.rsv = 4;
    QByteArray array = str;
    array += di;
    array += dio;
    array += _digital_out.raw;
    array += _digital_io_out.raw;
    return array;
}
