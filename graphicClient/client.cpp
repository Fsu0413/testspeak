#include "client.h"
#include <QByteArray>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QTcpSocket>

QPointer<QTcpSocket> socket;

extern QVariantMap config;
extern QVariantMap currentTlset;

QString selfName;

const Client::ClientFunction Client::ClientFunctions[CP_Max]
    = {nullptr, &Client::notifiedSignedIn, &Client::notifiedSignedOut, &Client::notifiedQueryResult, &Client::notifiedSpoken};

Client::Client(QObject *parent)
    : QObject(parent)
{
}

inline void writeJsonDocument(const QJsonDocument &doc)
{
    QByteArray arr = doc.toJson(QJsonDocument::Compact);
    arr.append("\n");
    socket->write(arr);
    socket->flush();
}

void Client::connectToHost(const QString &host, int port)
{
    if (socket)
        socket->deleteLater();

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &Client::signIn);
    connect(socket, &QTcpSocket::readyRead, this, &Client::socketReadyRead);

    socket->connectToHost(host, port);
}

void Client::disconnectFromHost()
{
    socket->disconnectFromHost();
}

void Client::notifiedSignedIn(const QJsonObject &contents)
{
    emit addPlayer(contents.value("name").toString());
}

void Client::notifiedSignedOut(const QJsonObject &contents)
{
    emit removePlayer(contents.value("name").toString());
}

void Client::notifiedQueryResult(const QJsonObject &contents)
{
    if (contents.contains("res")) {
        QJsonArray arr = contents.value("res").toArray();
        QVariantList list = arr.toVariantList();
        foreach (const QVariant &var, list) {
            QString str = var.toString();
            if (str != selfName)
                emit addPlayer(str);
        }
    } else {
        emit playerDetail(contents);
    }
}

void Client::notifiedSpoken(const QJsonObject &contents)
{
    QString from = contents.value("from").toString();
    QString to;
    if (contents.contains("to"))
        to = contents.value("to").toString();
    bool fromyou = (from == selfName);
    bool groupsent = to.isEmpty();
    bool toyou = false;
    if (!groupsent)
        toyou = (to == selfName);
    QString content = contents.value("content").toString();

    emit playerSpoken(from, to, content, fromyou, toyou, groupsent);
}

void Client::signIn()
{
    if (config.contains("tlset")) {
        // currentTlSet has been read by readConfig
        QJsonObject ob = QJsonObject::fromVariantMap(currentTlset);
        ob.remove("comments");
        ob.remove("aiFile");
        ob.remove("key");
        selfName = ob.value("name").toString();
        ob["protocolValue"] = int(SP_SignIn);
        QJsonDocument doc(ob);
        writeJsonDocument(doc);

        // read current online player
        QJsonObject ob2;
        ob2["protocolValue"] = int(SP_Query);
        QJsonDocument doc2(ob2);
        writeJsonDocument(doc2);
    }
}

void Client::socketReadyRead()
{
    while (socket->canReadLine()) {
        QByteArray line = socket->readLine();
        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (doc.isObject()) {
            int protocolValue = doc.object().value(QStringLiteral("protocolValue")).toInt();
            if (protocolValue > CP_Zero && protocolValue < CP_Max) {
                QJsonObject content = doc.object();
                (this->*(ClientFunctions[protocolValue]))(content);
            }
        }
    }
}

void Client::speak(QString to, QString content)
{
    QJsonObject ob;
    ob["from"] = selfName;
    if (!to.isEmpty())
        ob["to"] = to;
    ob["content"] = content;
    ob["protocolValue"] = int(SP_Speak);
    QJsonDocument doc(ob);
    writeJsonDocument(doc);
}

void Client::queryPlayerDetail(QString name)
{
    QJsonObject ob2;
    ob2["protocolValue"] = int(SP_Query);
    ob2["name"] = name;
    QJsonDocument doc2(ob2);
    writeJsonDocument(doc2);
}
