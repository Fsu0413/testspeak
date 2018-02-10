#include "server.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

enum ServerProtocol
{
    SP_Zero,

    SP_SignIn,
    SP_Query,
    SP_Speak,

    SP_Max,
};

enum ClientProtocol
{
    CP_Zero,

    CP_SignedIn,
    CP_SignedOut,
    CP_QueryResult,
    CP_Spoken,

    CP_Max,
};

inline void writeJsonDocument(QTcpSocket *socket, const QJsonDocument &doc)
{
    QByteArray arr = doc.toJson(QJsonDocument::Compact);
    arr.append("\n");
    socket->write(arr);
    socket->flush();
}

class ServerImpl : public QObject
{
    Q_OBJECT

private:
    QTcpServer *server;
    QMap<QTcpSocket *, QJsonObject> socketTlMap;
    QMap<QString, QTcpSocket *> nameSocketMap;

    typedef void (ServerImpl::*ServerFunction)(QTcpSocket *socket, const QJsonObject &content);

    static const ServerFunction ServerFunctions[SP_Max];

public:
    ServerImpl()
        : QObject(qApp)
    {
        server = new QTcpServer(this);
        connect(server, &QTcpServer::newConnection, this, &ServerImpl::incomingConnection);
    }

    void listen()
    {
        server->listen(QHostAddress::Any, 40001);
    }

public:
    void signInFunc(QTcpSocket *socket, const QJsonObject &content)
    {
        socketTlMap[socket] = content;
        auto copy = content;
        copy[QStringLiteral("protocolValue")] = int(CP_SignedIn);

        QJsonDocument doc(copy);

        foreach (QTcpSocket *to, nameSocketMap)
            writeJsonDocument(to, doc);

        QString name = content.value("name").toString();
        nameSocketMap[name] = socket;

        qDebug() << socket->peerAddress().toString() << "," << socket->peerPort() << "registered as" << name;
    }

    void queryFunc(QTcpSocket *socket, const QJsonObject &content)
    {
        QJsonObject ret;
        if (!content.contains("name")) {
            qDebug() << socket->peerAddress().toString() << "," << socket->peerPort() << "is querying all";
            QJsonArray arr;
            foreach (const QString &name, nameSocketMap.keys())
                arr.append(name);

            ret[QStringLiteral("res")] = arr;
        } else {
            QString name = content.value("name").toString();
            qDebug() << socket->peerAddress().toString() << "," << socket->peerPort() << "is querying" << name;
            if (nameSocketMap.contains(name)) {
                QTcpSocket *socket = nameSocketMap.value(name);
                ret = socketTlMap.value(socket);
            }
        }
        ret[QStringLiteral("protocolValue")] = int(CP_QueryResult);

        QJsonDocument doc(ret);
        writeJsonDocument(socket, doc);
    }

    void speakFunc(QTcpSocket *socket, const QJsonObject &content)
    {
        qDebug() << socket->peerAddress().toString() << "," << socket->peerPort() << "is speaking";

        QList<QTcpSocket *> tos = nameSocketMap.values();

        if (content.contains(QStringLiteral("to"))) {
            QString name = content.value("to").toString();
            qDebug() << "to" << name;
            if (nameSocketMap.contains(name)) {
                tos.clear();
                tos << socket << nameSocketMap[name];
            } else {
                return;
            }
        }

        auto copy = content;
        copy[QStringLiteral("protocolValue")] = int(CP_Spoken);
        copy[QStringLiteral("time")] = QDateTime::currentDateTime().toTime_t();

        QJsonDocument doc(copy);
        foreach (QTcpSocket *to, tos)
            writeJsonDocument(to, doc);
    }

public slots:
    void incomingConnection()
    {
        while (server->hasPendingConnections()) {
            QTcpSocket *socket = server->nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, &ServerImpl::socketCanRead);
            connect(socket, &QTcpSocket::disconnected, this, &ServerImpl::socketDisconnected);
            qDebug() << socket->peerAddress().toString() << "," << socket->peerPort() << "connected.";
        }
    }

    void socketCanRead()
    {
        QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
        if (socket == nullptr)
            return;

        while (socket->canReadLine()) {
            QByteArray line = socket->readLine();
            QJsonDocument doc = QJsonDocument::fromJson(line);
            if (doc.isObject()) {
                int protocolValue = doc.object().value(QStringLiteral("protocolValue")).toInt();
                qDebug() << socket->peerAddress().toString() << "," << socket->peerPort() << "sent protocol value" << protocolValue;
                if (protocolValue > SP_Zero && protocolValue < SP_Max) {
                    QJsonObject content = doc.object();
                    (this->*(ServerFunctions[protocolValue]))(socket, content);
                }
            }
        }
    }

    void socketDisconnected()
    {
        QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
        if (socket != nullptr) {
            qDebug() << socket->peerAddress().toString() << "," << socket->peerPort() << "disconnected.";
            QString name = socketTlMap[socket].value(QStringLiteral("name")).toString();
            socketTlMap.remove(socket);
            nameSocketMap.remove(name);

            QJsonObject ob;
            ob["protocolValue"] = int(CP_SignedOut);
            ob["name"] = name;

            QJsonDocument doc(ob);

            foreach (QTcpSocket *socket, nameSocketMap)
                writeJsonDocument(socket, doc);
        }
    }
};

const ServerImpl::ServerFunction ServerImpl::ServerFunctions[SP_Max] = {nullptr, &ServerImpl::signInFunc, &ServerImpl::queryFunc, &ServerImpl::speakFunc};

Server::Server()
{
    (new ServerImpl)->listen();
}

#include "server.moc"
