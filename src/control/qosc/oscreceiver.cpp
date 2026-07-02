#include "oscreceiver.h"
#include "contrib/oscpack/OscTypes.h"
#include "contrib/oscpack/OscReceivedElements.h"
#include "contrib/oscpack/OscException.h"

OscReceiver::OscReceiver(quint16 receivePort, QObject* parent) :
        QObject(parent)
{
    m_udpSocket = new QUdpSocket(this);
    // m_udpSocket->bind(QHostAddress::LocalHost, receivePort);
    qDebug() << "Listening for OSC on " << receivePort;
    m_udpSocket->bind(QHostAddress::Any, receivePort);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &OscReceiver::readyReadCb);
}

void OscReceiver::readyReadCb() {
    while (this->m_udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = this->m_udpSocket->receiveDatagram();
        // XXX: we could also retrieve the sender host and port
        QByteArray data = datagram.data();
        QVariantList arguments;
        QString oscAddress;
        // A malformed datagram must never take the app down: only forward the
        // message when it parsed cleanly, and keep draining the queue otherwise.
        if (this->byteArrayToVariantList(arguments, oscAddress, data)) {
            emit messageReceived(oscAddress, arguments);
            qDebug() << "C++OscReceiver Received: " << oscAddress << arguments;
        }
    }
}

bool OscReceiver::byteArrayToVariantList(QVariantList& outputVariantList, QString& outputOscAddress, const QByteArray& inputByteArray) {
    // oscpack throws osc::Exception (derived from std::exception) on any
    // malformed input. Catch it here so a single bad datagram is dropped
    // rather than unwinding through the Qt event loop.
    try {
        osc::ReceivedPacket packet(inputByteArray.data(), static_cast<std::size_t>(inputByteArray.size()));
        if (packet.IsMessage()) {
            osc::ReceivedMessage message(packet);
            // Get address pattern
            QString address(message.AddressPattern());
            outputOscAddress.replace(0, address.size(), address);

            for (auto iter = message.ArgumentsBegin(); iter != message.ArgumentsEnd(); ++ iter) {
                osc::ReceivedMessageArgument argument = (*iter);
                if (argument.IsBool()) {
                    outputVariantList.append(QVariant(argument.AsBool()));
                } else if (argument.IsString()) {
                    outputVariantList.append(QVariant(argument.AsString()));
                } else if (argument.IsInt32()) {
                    outputVariantList.append(QVariant::fromValue<qint32>(static_cast<qint32>(argument.AsInt32())));
                } else if (argument.IsFloat()) {
                    outputVariantList.append(QVariant(argument.AsFloat()));
                } else if (argument.IsChar()) {
                    outputVariantList.append(QVariant(argument.AsChar()));
                //} else if (argument.IsInt64()) {
                //    outputVariantList.append(QVariant(argument.AsInt64()));
                } else if (argument.IsDouble()) {
                    outputVariantList.append(QVariant(argument.AsDouble()));
                }
                // TODO: support Array, Midi, Blob, Symbol, TimeTag, RGBA, Nil
            }
        } // TODO: also parse bundles
    } catch (const osc::Exception& e) {
        qWarning() << "OscReceiver: dropped malformed OSC packet:" << e.what();
        return false;
    }
    return true;
}
