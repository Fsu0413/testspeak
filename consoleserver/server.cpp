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
#include <QTimer>

enum ServerProtocol
{
    SP_Zero,

    SP_SignIn,
    SP_Query,
    SP_Speak,
    SP_SignOut,

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

class ServerImpl : public QObject
{
    Q_OBJECT

private:
    QTcpServer *server;
    QMap<QString, QTcpSocket *> nameSocketMap;
    QMap<QString, QJsonObject> nameTlMap;
    QMap<QString, QList<QJsonDocument>> disconnectCache;
    QMap<QString, QTimer *> disconnectTimerMap;

    typedef void (ServerImpl::*ServerFunction)(QTcpSocket *socket, const QJsonObject &content);

    static const ServerFunction ServerFunctions[SP_Max];

    inline void writeJsonDocument(const QString &name, const QJsonDocument &doc)
    {
        QTcpSocket *socket = nameSocketMap[name];
        if (socket != nullptr) {
            QByteArray arr = doc.toJson(QJsonDocument::Compact);
            arr.append("\n");
            socket->write(arr);
            socket->flush();
        } else {
            (void)disconnectCache[name];
            disconnectCache[name] << doc;
        }
    }

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

    void doSignOut(const QString &name)
    {
        nameSocketMap.remove(name);
        nameTlMap.remove(name);
        disconnectCache.remove(name);
        disconnectTimerMap.remove(name);

        QJsonObject ob;
        ob["protocolValue"] = int(CP_SignedOut);
        ob["userName"] = name;

        QJsonDocument doc(ob);

        foreach (const QString &socket, nameSocketMap.keys())
            writeJsonDocument(socket, doc);
    }

public:
    void signInFunc(QTcpSocket *socket, const QJsonObject &content)
    {
        QString name = content.value("userName").toString();
        bool reconnect = false;
        if (!disconnectTimerMap.contains(name)) {
            auto copy = content;
            copy[QStringLiteral("protocolValue")] = int(CP_SignedIn);

            QJsonDocument doc(copy);

            foreach (const QString &name, nameSocketMap.keys()) {
                if (!disconnectTimerMap.contains(name))
                    writeJsonDocument(name, doc);
            }
            qDebug() << socket->peerAddress().toString() << "," << socket->peerPort() << "registered as" << name;
            nameTlMap[name] = content;
        } else {
            disconnectTimerMap[name]->stop();
            disconnectTimerMap[name]->deleteLater();
            disconnectTimerMap.remove(name);
            qDebug() << socket->peerAddress().toString() << "," << socket->peerPort() << "reconnected as" << name;
            reconnect = true;
        }

        nameSocketMap[name] = socket;
        queryFunc(socket, QJsonObject());

        if (reconnect && disconnectCache.contains(name)) {
            foreach (const QJsonDocument &doc, disconnectCache[name])
                writeJsonDocument(name, doc);

            disconnectCache.remove(name);
        }
    }

    void queryFunc(QTcpSocket *socket, const QJsonObject &content)
    {
        QJsonObject ret;
        QString name = nameSocketMap.key(socket);
        if (name.isEmpty())
            return;

        if (!content.contains("userName")) {
            qDebug() << socket->peerAddress().toString() << "," << socket->peerPort() << "is querying all";
            QJsonArray arr;
            QJsonObject ob;
            foreach (const QJsonObject &tl, nameTlMap) {
                arr.append(tl.value("userName"));
                ob[tl.value("userName").toString()] = tl;
            }
            ret[QStringLiteral("res")] = arr;
            ret[QStringLiteral("resv2")] = ob;

        } else {
            QString name = content.value("userName").toString();
            qDebug() << socket->peerAddress().toString() << "," << socket->peerPort() << "is querying" << name;
            if (nameTlMap.contains(name))
                ret = nameTlMap.value(name);
        }
        ret[QStringLiteral("protocolValue")] = int(CP_QueryResult);

        QJsonDocument doc(ret);
        writeJsonDocument(name, doc);
    }

    void speakFunc(QTcpSocket *socket, const QJsonObject &content)
    {
        qDebug() << socket->peerAddress().toString() << "," << socket->peerPort() << "is speaking";

        //QList<QTcpSocket *> tos = nameSocketMap.values();
        QStringList tos = nameSocketMap.keys();

        if (content.contains(QStringLiteral("to"))) {
            QString name = content.value("to").toString();
            qDebug() << "to" << name;
            if (nameSocketMap.contains(name)) {
                tos.clear();
                //tos << socket << nameSocketMap[name];
                tos << nameSocketMap.key(socket) << name;
            } else {
                return;
            }
        }

        auto copy = content;
        copy[QStringLiteral("protocolValue")] = int(CP_Spoken);
        copy[QStringLiteral("time")] = qint64(QDateTime::currentDateTime().toTime_t());

        QJsonDocument doc(copy);
        foreach (const QString &to, tos)
            writeJsonDocument(to, doc);
    }

    void signOutFunc(QTcpSocket *socket, const QJsonObject &content)
    {
        (void)content;
        qDebug() << socket->peerAddress().toString() << "," << socket->peerPort() << "signed out.";
        QString name = nameSocketMap.key(socket);
        if (name.isEmpty())
            return;

        disconnect(socket);
        socket->disconnectFromHost();
        // socket->deleteLater();

        doSignOut(name);
    }

    void heartbeatFunc(QTcpSocket *socket, const QJsonObject &content)
    {
        (void)content;
        QJsonObject ob;
        ob[QStringLiteral("protocolValue")] = int(CP_Zero);

        QJsonDocument doc(ob);
        writeJsonDocument(nameSocketMap.key(socket), doc);
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
                if (protocolValue >= SP_Zero && protocolValue < SP_Max) {
                    QJsonObject content = doc.object();
                    auto func = ServerFunctions[protocolValue];

                    if (func != nullptr)
                        (this->*func)(socket, content);
                }
            }
        }
    }

    void socketDisconnected()
    {
        QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
        if (socket != nullptr) {
            qDebug() << socket->peerAddress().toString() << "," << socket->peerPort() << "disconnected.";
            QString name = nameSocketMap.key(socket);
            if (name.isEmpty())
                return;

            nameSocketMap[name] = nullptr;

            QTimer *t = new QTimer(this);
            t->setInterval(180000);
            t->setSingleShot(true);
            connect(t, &QTimer::timeout, this, &ServerImpl::disconnectTimerTimeout);

            disconnectTimerMap[name] = t;
            t->start();
        }
    }

    void disconnectTimerTimeout()
    {
        QTimer *t = qobject_cast<QTimer *>(sender());
        if (t != nullptr) {
            QString name = disconnectTimerMap.key(t);
            if (name.isEmpty())
                return;

            doSignOut(name);
        }
    }
};

// clang-format off
const ServerImpl::ServerFunction ServerImpl::ServerFunctions[SP_Max] = {
    &ServerImpl::heartbeatFunc,
    &ServerImpl::signInFunc,
    &ServerImpl::queryFunc,
    &ServerImpl::speakFunc,
    &ServerImpl::signOutFunc,
};
// clang-format on

Server::Server()
{
    (new ServerImpl)->listen();
}

#include "server.moc"
