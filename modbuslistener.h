#ifndef MODBUSLISTENER_H
#define MODBUSLISTENER_H

#include <QModbusDataUnit>
#include <QMainWindow>
#include <QTimer>

#include <settingsdialog.h>
QT_BEGIN_NAMESPACE

class QModbusClient;
class QModbusReply;

namespace Ui {
class SettingsDialog;
}

class ModbusListener : public QMainWindow
{
    Q_OBJECT
public:
    explicit ModbusListener(QWidget *parent = nullptr);
    ~ModbusListener();
    QModbusDataUnit readRequest() const;

    bool isModbusConnected();
    bool isModbusAlive();

    const QModbusDataUnit *unit;
    QStringList *strList;
    double voltagePhaseA;
    double voltagePhaseB;
    double voltagePhaseC;

    double currentPhaseA;
    double currentPhaseB;
    double currentPhaseC;

    double frequency;

    float temperature;

    SettingsDialog *m_settingsDialog;
    QModbusClient *modbusDevice;

    QString     portName;
private:
    bool        m_connected;
    bool        _alive;

    QTimer      *timer;
    int         slaveNumber;
    void initActions();
    QModbusReply *lastRequest;
public slots:
    void on_connectButton_clicked();
    void updateSlaveNumber( int number );
    void reconnectModbus();
private slots:
    void onStateChanged(int state);
    void on_readButton_clicked();
    void readReady();
    void on_connectType_currentIndexChanged(int);
signals:
    void getReply();
    void getTemperature();
    void isConnected(bool);
};

#endif // MODBUSLISTENER_H
