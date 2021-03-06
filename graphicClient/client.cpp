#include "client.h"
#include <QByteArray>
#include <QDebug>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QTcpSocket>
#include <QTimer>

QPointer<QTcpSocket> socket;

extern QVariantMap config;
extern QVariantMap currentTlset;

QString selfName;

QHostAddress addr;
quint16 port;
bool signedOn = false;
bool signedOff = false;
bool disconnected = false;
QList<QJsonDocument> disconnectPendingMessages;

// clang-format off
const Client::ClientFunction Client::ClientFunctions[CP_Max] = {
    &Client::notifiedHeartBeat,
    &Client::notifiedSignedIn,
    &Client::notifiedSignedOut,
    &Client::notifiedQueryResult,
    &Client::notifiedSpoken
};
// clang-format on

Client::Client(QObject *parent)
    : QObject(parent)
{
}

void Client::writeJsonDocument(const QJsonDocument &doc)
{
    if (signedOn && !disconnected) {
        QByteArray arr = doc.toJson(QJsonDocument::Compact);
        arr.append("\n");
        socket->write(arr);
        socket->flush();
    } else {
        disconnectPendingMessages << doc;
    }
}

void Client::resendMessage()
{
    foreach (const QJsonDocument &doc, disconnectPendingMessages)
        writeJsonDocument(doc);

    disconnectPendingMessages.clear();
}

void Client::connectToHost(const QString &host, int port)
{
    if (socket)
        socket->deleteLater();

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &Client::signIn);
    connect(socket, &QTcpSocket::readyRead, this, &Client::socketReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Client::lostConnection);
    connect(socket, (void (QTcpSocket::*)(QAbstractSocket::SocketError))(&QTcpSocket::error), this, &Client::socketError);

    addr = host;
    ::port = port;

    socket->connectToHost(host, port);
}

void Client::disconnectFromHost()
{
    signedOff = true;
    QJsonObject ob;
    ob[QStringLiteral("protocolValue")] = int(SP_SignOut);
    QJsonDocument doc(ob);
    writeJsonDocument(doc);
    socket->disconnectFromHost();
}

void Client::notifiedHeartBeat(const QJsonObject &contents)
{
    (void)contents;
}

void Client::notifiedSignedIn(const QJsonObject &contents)
{
    emit addPlayer(contents.value(QStringLiteral("userName")).toString(), contents.value(QStringLiteral("gender")).toString());
}

void Client::notifiedSignedOut(const QJsonObject &contents)
{
    emit removePlayer(contents.value(QStringLiteral("userName")).toString());
}

void Client::notifiedQueryResult(const QJsonObject &contents)
{
    if (contents.contains(QStringLiteral("resv2"))) {
        QJsonObject ob = contents.value(QStringLiteral("resv2")).toObject();
        foreach (const QString &str, ob.keys()) {
            if (str != selfName) {
                QString gender = ob.value(str).toObject().value(QStringLiteral("gender")).toString();
                emit addPlayer(str, gender);
            }
        }
    } else {
        emit playerDetail(contents);
    }
}

void Client::notifiedSpoken(const QJsonObject &contents)
{
    QString from = contents.value(QStringLiteral("from")).toString();
    QString to;
    if (contents.contains(QStringLiteral("to")))
        to = contents.value(QStringLiteral("to")).toString();
    bool fromyou = (from == selfName);
    bool groupsent = to.isEmpty();
    bool toyou = false;
    if (!groupsent)
        toyou = (to == selfName);
    QString content = contents.value(QStringLiteral("content")).toString();
    quint32 time = quint32(contents.value(QStringLiteral("time")).toDouble());

    emit playerSpoken(from, to, content, fromyou, toyou, groupsent, time);
}

void Client::signIn()
{
    disconnected = false;
    if (config.contains(QStringLiteral("tlset"))) {
        // currentTlSet has been read by readConfig
        QJsonObject ob = QJsonObject::fromVariantMap(currentTlset);
        ob.remove(QStringLiteral("comments"));
        ob.remove(QStringLiteral("aiFile"));
        ob.remove(QStringLiteral("key"));
        selfName = ob.value(QStringLiteral("userName")).toString();
        ob[QStringLiteral("protocolValue")] = int(SP_SignIn);
        QJsonDocument doc(ob);
        writeJsonDocument(doc);
    }

    if (!signedOn) {
        signedOn = true;
        QTimer *heartbeatTimer = new QTimer(this);
        heartbeatTimer->setInterval(5000);
        heartbeatTimer->setSingleShot(false);
        connect(heartbeatTimer, &QTimer::timeout, this, &Client::sendHeartBeat);
        heartbeatTimer->start();

        emit signedIn();
    }

    resendMessage();
}

void Client::lostConnection()
{
    if (socket != nullptr && !signedOff && sender() == socket) {
        disconnected = true;
        socket->deleteLater();

        socket = new QTcpSocket(this);
        connect(socket, &QTcpSocket::connected, this, &Client::signIn);
        connect(socket, &QTcpSocket::readyRead, this, &Client::socketReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &Client::lostConnection);
        connect(socket, (void (QTcpSocket::*)(QAbstractSocket::SocketError))(&QTcpSocket::error), this, &Client::socketError);

        socket->connectToHost(addr, port);
    }
}

void Client::socketError(QAbstractSocket::SocketError e)
{
    (void)e;
    lostConnection();
}

void Client::socketReadyRead()
{
    while (socket->canReadLine()) {
        QByteArray line = socket->readLine();
        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (doc.isObject()) {
            int protocolValue = doc.object().value(QStringLiteral("protocolValue")).toInt();
            if (protocolValue >= CP_Zero && protocolValue < CP_Max) {
                QJsonObject content = doc.object();
                auto func = ClientFunctions[protocolValue];
                if (func != nullptr)
                    (this->*func)(content);
            }
        }
    }
}

void Client::speak(QString to, QString content)
{
    QJsonObject ob;
    ob[QStringLiteral("from")] = selfName;
    if (!to.isEmpty())
        ob[QStringLiteral("to")] = to;
    ob[QStringLiteral("content")] = content;
    ob[QStringLiteral("protocolValue")] = int(SP_Speak);
    QJsonDocument doc(ob);
    writeJsonDocument(doc);
}

void Client::sendHeartBeat()
{
    if (!disconnected && !socket.isNull()) {
        QJsonObject ob;
        ob[QStringLiteral("protocolValue")] = int(SP_Zero);
        QJsonDocument doc(ob);
        writeJsonDocument(doc);
    }
}

void Client::queryPlayerDetail(QString name)
{
    QJsonObject ob2;
    ob2[QStringLiteral("protocolValue")] = int(SP_Query);
    ob2[QStringLiteral("userName")] = name;
    QJsonDocument doc2(ob2);
    writeJsonDocument(doc2);
}
