#include "dialog.h"

#include <QDebug>
#include <QFile>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpServer>
#include <QTimer>
#include <QtAlgorithms>
#include <QtMath>

struct tlData
{
    tlData(QString key, QString userId)
        : key(key)
        , userId(userId)
    {
    }

    QString key;
    QString userId;
};

QList<tlData> TLData;

Dialog::Dialog(QWidget *parent)
    : QWidget(parent)
    , socket(nullptr)
{
    TLData << tlData(QStringLiteral("884b7214b1774e74963fa30f468126aa"), QStringLiteral("test1"));
    TLData << tlData(QStringLiteral("15ba950f34fb437dbfc1ab35c7b23949"), QStringLiteral("test2"));
    TLData << tlData(QStringLiteral("9b9b40047d9c43e8b4789c0f35ae78c1"), QStringLiteral("test3"));
    TLData << tlData(QStringLiteral("93582ff93fe64a69b5832c2d6519d868"), QStringLiteral("test4"));
    TLData << tlData(QStringLiteral("f3ebf25d77084a63a0f874c83f2c7418"), QStringLiteral("test5"));

    nam1 = new QNetworkAccessManager(this);
    //QTimer::singleShot(5000, this, SLOT(send1stPack()));

    listWidget = new QListWidget;
    listWidget->setSortingEnabled(false);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(listWidget);

    setLayout(layout);

    QTcpServer *server = new QTcpServer(this);
    server->listen(QHostAddress::Any, 40000);
    connect(server, &QTcpServer::newConnection, this, &Dialog::handleNewConnection);
}

Dialog::~Dialog()
{
}

void Dialog::sendPack()
{
    QString x = QStringLiteral("received: ");
    x.append(str);
    listWidget->addItem(x);
    listWidget->scrollToBottom();

    QJsonObject ob;

    ob["key"] = QJsonValue(TLData[to].key);
    ob["userid"] = QJsonValue(TLData[to].userId);
    ob["info"] = QJsonValue(str);

    QJsonDocument doc(ob);

    QNetworkRequest req(QUrl(QStringLiteral("http://www.tuling123.com/openapi/api")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("text/json"));

    QNetworkReply *reply = nam1->post(req, doc.toJson());
    connect(reply, &QNetworkReply::finished, this, &Dialog::receive);
}

void Dialog::setStuckString()
{
    QFile f(QString(":/x/change.txt"));
    f.open(QFile::ReadOnly);
    str = QString::fromUtf8(f.readAll());
    f.close();
}

void Dialog::send1stPack()
{
    setStuckString();

    to = 2;
    sendPack();
}

void Dialog::handleNewConnection()
{
    QTcpServer *server = qobject_cast<QTcpServer *>(sender());
    if (server != nullptr) {
        socket = server->nextPendingConnection();
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
        connect(socket, &QTcpSocket::readyRead, this, &Dialog::receiveFromClient);
        //send1stPack();
    }
}

const int delay = 0; //1000;

void Dialog::receive()
{
    //    if (qrand() % 10 < 1) {
    //        do {
    //            to = qrand() % TLData.length();
    //        } while (to == from);
    //    }

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (reply == nullptr)
        return;

    QByteArray arr = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(arr);
    if (doc.isObject()) {
        QJsonObject ob = doc.object();
        if (ob.contains(QStringLiteral("code"))) {
            QJsonValue value = ob.value(QStringLiteral("code"));
            int x = value.toInt();
            switch (x) {
            case 100000: {
                str = ob.value("text").toString();
                QTimer::singleShot(delay, this, SLOT(sendToClient()));
                break;
            }
            default: {
                qDebug() << x;
                setStuckString();
                QTimer::singleShot(delay, this, SLOT(sendToClient()));
            }
            }
        }
    }
}

void Dialog::receiveFromClient()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket != nullptr) {
        while (socket->canReadLine()) {
            str = socket->readLine();
            to = 2;
            sendPack();
        }
    }
}

void Dialog::sendToClient()
{
    if (socket != nullptr) {
        QString x = QStringLiteral("sent: ");
        x.append(str);
        listWidget->addItem(x);
        listWidget->scrollToBottom();

        QString toSend = str + QStringLiteral("\n");
        socket->write(toSend.toUtf8());
        socket->flush();
    }
}
