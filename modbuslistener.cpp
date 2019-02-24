#include "modbuslistener.h"
#include "ui_modbuslistener.h"
#include "settingsdialog.h"

#include <QModbusRtuSerialMaster>
#include <QStandardItemModel>
#include <QStatusBar>

#include <QDebug>
#include <QPointer>

enum ModbusConnection {
    Serial
};

ModbusListener::ModbusListener(QWidget *parent)
    : QMainWindow(parent)
//    , ui(new Ui::ModbusListener)
    , lastRequest(nullptr)
    , modbusDevice(nullptr)
{
    slaveNumber = 0;
    timer = new QTimer();
    timer->setInterval(500);
    connect( timer, &QTimer::timeout, this, &ModbusListener::on_readButton_clicked );

    m_settingsDialog = new SettingsDialog(this);
    on_connectType_currentIndexChanged(0);

    strList = new QStringList();

}

ModbusListener::~ModbusListener()
{
    if (modbusDevice)
        modbusDevice->disconnectDevice();
    delete modbusDevice;
}

void ModbusListener::on_connectType_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (modbusDevice) {
        modbusDevice->disconnectDevice();
        delete modbusDevice;
        modbusDevice = nullptr;
    }

    modbusDevice = new QModbusRtuSerialMaster(this);

    connect(modbusDevice, &QModbusClient::errorOccurred, [this](QModbusDevice::Error) {
        statusBar()->showMessage(modbusDevice->errorString(), 5000); } );

    if (!modbusDevice)
    {
        statusBar()->showMessage(tr("Could not create Modbus master."), 5000);
    } else {
        connect(modbusDevice, &QModbusClient::stateChanged,
                this, &ModbusListener::onStateChanged);
    }
}

void ModbusListener::on_connectButton_clicked()
{
    if (!modbusDevice)
        return;

    if (modbusDevice->state() != QModbusDevice::ConnectedState)
    {
        modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter,
            portName );

        modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter,
            m_settingsDialog->settings().parity);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,
            m_settingsDialog->settings().baud);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,
            m_settingsDialog->settings().dataBits);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,
            m_settingsDialog->settings().stopBits);

        modbusDevice->setTimeout(m_settingsDialog->settings().responseTime);
        modbusDevice->setNumberOfRetries(m_settingsDialog->settings().numberOfRetries);
        if (!modbusDevice->connectDevice())
        {
            statusBar()->showMessage(tr("Connect failed: ") + modbusDevice->errorString(), 5000);
        } else {
            timer->start();
        }
    } else {
        modbusDevice->disconnectDevice();
    }
}

void ModbusListener::onStateChanged(int state)
{
    bool connected = (state != QModbusDevice::UnconnectedState);
    emit isConnected(connected);
}

void ModbusListener::on_readButton_clicked()
{
    if (!modbusDevice)
        return;
    statusBar()->clearMessage();

    if (auto *reply = modbusDevice->sendReadRequest(readRequest(), slaveNumber ) ) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &ModbusListener::readReady);
        else
            delete reply; // broadcast replies return immediately
    } else {
        statusBar()->showMessage(tr("Read error: ") + modbusDevice->errorString(), 5000);
    }
}

void ModbusListener::readReady()
{
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
        return;

    if (reply->error() == QModbusDevice::NoError)
    {
        const QModbusDataUnit unit = reply->result();
        strList->clear();
        for (uint i = 0; i < unit.valueCount(); i++)
        {
            const QString entry = tr("%1").arg(QString::number(unit.value(i), 10 ));
            strList->append( entry );
            if ( i == 34 )
                voltagePhaseA = entry.toDouble() / 10;
            if ( i == 35)
                voltagePhaseB = entry.toDouble() / 10;
            if ( i == 36)
                voltagePhaseC = entry.toDouble() / 10;
            if ( i == 40 )
                currentPhaseA = entry.toDouble() * 8 / 1000;
            if ( i == 43 )
                frequency = entry.toDouble() / 100;

//            qDebug() << entry;

        }
        emit getReply();

    } else if (reply->error() == QModbusDevice::ProtocolError) {
        statusBar()->showMessage(tr("Read response error: %1 (Mobus exception: 0x%2)").
                                    arg(reply->errorString()).
                                    arg(reply->rawResult().exceptionCode(), -1, 16), 5000);
    } else {
        statusBar()->showMessage(tr("Read response error: %1 (code: 0x%2)").
                                    arg(reply->errorString()).
                                    arg(reply->error(), -1, 16), 5000);
    }

    reply->deleteLater();
}

QModbusDataUnit ModbusListener::readRequest() const
{
    const auto table = QModbusDataUnit::HoldingRegisters;
    int startAddress = 0;
    int numberOfEntries = 50;
    return QModbusDataUnit(table, startAddress, numberOfEntries);
}

void ModbusListener::updateSlaveNumber( int number )
{
    slaveNumber = number;
}
