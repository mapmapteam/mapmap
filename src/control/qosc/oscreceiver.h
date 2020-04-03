#ifndef OSCRECEIVER_H
#define OSCRECEIVER_H

#include <QObject>
#include <QVariant>
#include <QtNetwork>
#include <QHostAddress>
#include "qosc_global.h"

/**
 * @brief Receives OSC on a given port number.
 *
 * Currently only supports unicast UDP.
 */
class QOSC_EXPORT OscReceiver : public QObject
{
    Q_OBJECT

    // TODO: Support multicast UDP
    // TODO: Support TCP
    // TODO: Support broadcast UDP
    // TODO: Support OSC bundles
    // TODO: Provide a class to route messages.
public:
    /**
     * @brief Constructor.
     * @param receivePort Port number to listen to.
     */
    explicit OscReceiver(quint16 receivePort, QObject *parent = nullptr);

signals:
    /**
     * @brief Signal triggered each time we receive a message.
     * @param oscAddress
     * @param message
     */
    void messageReceived(const QString& oscAddress, const QVariantList& message);

public slots:
    void readyReadCb();

private:
    QUdpSocket* m_udpSocket;
    void byteArrayToVariantList(QVariantList& outputVariantList, QString& outputOscAddress, const QByteArray& inputByteArray);
};

#endif // OSCRECEIVER_H
