#include "dialog.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpServer>
#include <QTimer>
#include <QVBoxLayout>
#include <QtAlgorithms>
#include <QtMath>
#include <random>

const int delay = 3000; //1000;
const int typedelay = 200;
const int sendDelay = 500;

std::random_device rd;

int generateRandom(int in)
{
    int min = in * 0.3;
    int max = in * 1.5;
    int diff = max - min;

    return (rd() % diff) + min;
}

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

void Dialog::getStringFromFile(const QString &fileName)
{
    QFile f(fileName);
    f.open(QFile::ReadOnly);
    QString s = QString::fromUtf8(f.readAll());
    f.close();

    QStringList sl = s.split("\n", QString::SkipEmptyParts);
    str = sl.at(rd() % sl.length()).trimmed();
}

QList<tlData> TLData;

Dialog::Dialog(QWidget *parent)
    : QWidget(parent)
    , socket(nullptr)
{
    TLData << tlData(QStringLiteral("e05c7e3c40544876896bc1312802a693"), QStringLiteral("rp"));
    TLData << tlData(QStringLiteral("c0edfa86336345e4b33e706c704aa946"), QStringLiteral("cw"));
    TLData << tlData(QStringLiteral("346af706007f40a29461f2bed2bed1d3"), QStringLiteral("ecy"));
    TLData << tlData(QStringLiteral("ca3f89d3017a4240833185349f1af003"), QStringLiteral("yx"));
    TLData << tlData(QStringLiteral("1607a0d5063943989accb204fdd51f13"), QStringLiteral("zr"));

    to = 3;

    nam1 = new QNetworkAccessManager(this);
    //QTimer::singleShot(5000, this, SLOT(send1stPack()));

    listWidget = new QListWidget;
    listWidget->setSortingEnabled(false);

    edit = new QLineEdit;
    edit->clear();

    str.clear();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(listWidget);
    layout->addWidget(edit);

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
    if (lastRecv == lastSent) {
        getStringFromFile(QStringLiteral(":/x/parrotdup.txt"));
        startTyping();
        return;
    }

    QJsonObject ob;

    ob["key"] = QJsonValue(TLData[to].key);
    ob["userid"] = QJsonValue(TLData[to].userId);
    ob["info"] = QJsonValue(lastRecv);

    QJsonDocument doc(ob);

    QNetworkRequest req(QUrl(QStringLiteral("http://www.tuling123.com/openapi/api")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("text/json"));

    QNetworkReply *reply = nam1->post(req, doc.toJson());
    connect(reply, &QNetworkReply::finished, this, &Dialog::receive);
}

void Dialog::setStuckString()
{
    getStringFromFile(QStringLiteral(":/x/change.txt"));
}

void Dialog::setGreetString()
{
    getStringFromFile(QStringLiteral(":/x/greet.txt"));
}

void Dialog::defeatDup()
{
    if (str == lastSent)
        getStringFromFile(QStringLiteral(":/x/senddup.txt"));
}

void Dialog::startTyping()
{
    if (str.isEmpty()) {
        int del = generateRandom(sendDelay);
        edit->selectAll();
        QTimer::singleShot(del, this, SLOT(sendToClient()));
    } else {
        int del = 0;
        if (edit->text().isEmpty())
            del = generateRandom(delay);
        else
            del = generateRandom(typedelay);
        QTimer::singleShot(del, this, SLOT(typing()));
    }
}

void Dialog::send1stPack()
{
    if (str.isEmpty()) {
        edit->clear();
        setGreetString();
    }

    startTyping();
}

void Dialog::handleNewConnection()
{
    QTcpServer *server = qobject_cast<QTcpServer *>(sender());
    if (server != nullptr) {
        socket = server->nextPendingConnection();
        qDebug() << "connected.";

        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
        connect(socket, &QTcpSocket::disconnected, []() { qDebug() << "disconnected."; });
        connect(socket, &QTcpSocket::readyRead, this, &Dialog::receiveFromClient);
        send1stPack();
    }
}

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

    str.clear();

    QJsonDocument doc = QJsonDocument::fromJson(arr);
    if (doc.isObject()) {
        QJsonObject ob = doc.object();
        if (ob.contains(QStringLiteral("code"))) {
            QJsonValue value = ob.value(QStringLiteral("code"));
            int x = value.toInt();
            switch (x) {
            case 100000: {
                str = ob.value("text").toString();
                str.replace(QRegExp("\\s"), QString());
                defeatDup();
                break;
            }
            default: {
                qDebug() << x;
            }
            }
        }
    }

    edit->clear();

    if (str.isEmpty())
        setStuckString();

    startTyping();
}

void Dialog::receiveFromClient()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket != nullptr) {
        while (socket->canReadLine()) {
            QString recv = socket->readLine().trimmed();

            QString x = QStringLiteral("received: ");
            x.append(recv);
            listWidget->addItem(x);
            listWidget->scrollToBottom();

            if (recv == lastRecv) {
                getStringFromFile(QStringLiteral(":/x/recvdup.txt"));
                startTyping();
            } else {
                lastRecv = recv;
                sendPack();
            }
        }
    }
}

void Dialog::sendToClient()
{
    if (socket != nullptr) {
        QString str = edit->text();
        QString x = QStringLiteral("sent: ");
        x.append(str);
        listWidget->addItem(x);
        listWidget->scrollToBottom();

        QString toSend = str + QStringLiteral("\n");
        socket->write(toSend.toUtf8());
        socket->flush();

        lastSent = str;

        edit->clear();
    }
}

void Dialog::typing()
{
    if (socket != nullptr) {
        if (!str.isEmpty()) {
            edit->setFocus();
            edit->deselect();

            QString left = str.left(1);
            str = str.mid(1);

            edit->setText(edit->text() + left);
            edit->setCursorPosition(edit->text().length());
        }
        startTyping();
    }
}
