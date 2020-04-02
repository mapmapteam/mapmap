#ifndef OSCSENDER_H
#define OSCSENDER_H

#include <QObject>
#include <QVariant>
#include <QtNetwork>
#include <QHostAddress>
#include "qosc_global.h"

/**
 * @brief Sends OSC messages to a given host and port.
 *
 * Currently only supports unicast UDP.
 */
class QOSC_EXPORT OscSender : public QObject
{
    Q_OBJECT

    // TODO: Add portNumber property (and allow users to change it)
    // TODO: Add hostAddress property (and allow users to change it)
    // TODO: Support TCP
    // TODO: Support multicast
    // TODO: Support broadcast
    // TODO: Support OSC bundles
    // TODO: Support DNS resolution

public:
    /**
     * @brief Constructor.
     * @param hostAddress
     * @param port
     * @param parent
     */
    explicit OscSender(const QString& hostAddress, quint16 port, QObject* parent = nullptr);

    /**
     * @brief Sends an OSC message to the host and address that this sender is configured to send to.
     * @param oscAddress OSC path /like/this
     * @param arguments List of QVariant arguments of any type
     */
    Q_INVOKABLE void send(const QString& oscAddress, const QVariantList& arguments);

signals:
    // TODO: Add messageSent signal
    // TODO: Add connected signal for TCP sender.

public slots:

private:
    QUdpSocket* m_udpSocket;
    QHostAddress m_hostAddress;
    quint16 m_port;

    void variantListToByteArray(QByteArray& outputResult, const QString& oscAddress, const QVariantList& arguments);
};

#endif // OSCSENDER_H
