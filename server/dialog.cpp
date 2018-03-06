#include "dialog.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
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

std::random_device rd;

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
    TLData << tlData(QStringLiteral("095669531b59423db7f615dfa84771f5"), QStringLiteral("v2"));

    to = 0;

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

    ob["reqType"] = 0;

    QJsonObject userInfo;
    userInfo["apiKey"] = QJsonValue(TLData[to].key);
    userInfo["userId"] = QJsonValue(TLData[to].userId);
    ob["userInfo"] = userInfo;

    QJsonObject perception;
    QJsonObject inputText;
    inputText["text"] = lastRecv;
    perception["inputText"] = inputText;
    ob["perception"] = perception;

    QJsonDocument doc(ob);

    QNetworkRequest req(QUrl(QStringLiteral("http://openapi.tuling123.com/openapi/api/v2")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

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
    if (str.isEmpty())
        QTimer::singleShot(1, this, SLOT(sendToClient()));
    else
        QTimer::singleShot(1, this, SLOT(typing()));
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
        QJsonObject intent = ob["intent"].toObject();
        int code = intent["code"].toInt();
        qDebug() << code;
        if (ob.contains("results")) {
            QJsonArray results = ob["results"].toArray();
            foreach (const QJsonValue &_value, results) {
                QJsonObject result = _value.toObject();
                qDebug() << result["resultType"].toString() << result["groupType"].toInt();
                if (result["resultType"].toString() == "text") {
                    QJsonObject values = result["values"].toObject();
                    str.append(values["text"].toString());
                    str.replace(QRegExp("\\s"), QString());
                }
            }
            if (!str.isEmpty())
                defeatDup();
        }
    }
    //        if (ob.contains(QStringLiteral("code"))) {
    //            QJsonValue value = ob.value(QStringLiteral("code"));
    //            int x = value.toInt();
    //            switch (x) {
    //            case 100000: {
    //                str = ob.value("text").toString();
    //                str.replace(QRegExp("\\s"), QString());
    //                defeatDup();
    //                break;
    //            }
    //            default: {
    //                qDebug() << x;
    //            }
    //            }
    //        }

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
