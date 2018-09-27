
#include "Ai.h"
#include "client.h"
#include "lua.hpp"

#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <QUrl>

extern QVariantMap config;
extern QVariantMap currentTlset;

extern "C" {
void luaopen_Ai(lua_State *l);
}

const QString packagePathStr = QStringLiteral("package.path = package.path .. \";%1/ai/lib/?.lua\"");

void setMe(lua_State *l, Ai *ai);

QVariantMap AiData;
QMutex AiDataMutex;

Ai::Ai(Dialog *dialog)
    : dialog(dialog)
    , nam1(nullptr)
    , l(nullptr)
{
}

Ai::~Ai()
{
    if (l != nullptr)
        lua_close(l);
}

void Ai::pcall(int argnum)
{
    if (lua_pcall(l, argnum, 0, 0) != LUA_OK) {
        QString err = QString::fromUtf8(lua_tostring(l, -1));
        qDebug() << err;
        lua_pop(l, 1);
    }
}

QString Ai::name()
{
    return currentTlset[QStringLiteral("name")].toString();
}

QString Ai::gender()
{
    return currentTlset[QStringLiteral("gender")].toString();
}

QString Ai::getPlayerGender(const QString &name)
{
    emit refreshPlayerList();

    QMutexLocker locker(&AiDataMutex);
    (void)locker;

    QVariantMap playerDetail = AiData.value(QStringLiteral("players")).toMap().value(name).toMap();
    return playerDetail.value(QStringLiteral("gender")).toString();
}

void Ai::queryTl(const QString &id, const QString &content, const QString &_key, const QString &aiComment)
{
    QJsonObject ob;

    QString key = _key;
    if (_key.isEmpty())
        key = currentTlset[QStringLiteral("key")].toString();

    ob[QStringLiteral("key")] = QJsonValue(key);
    ob[QStringLiteral("userid")] = QJsonValue(id);
    ob[QStringLiteral("info")] = QJsonValue(content);

    QJsonDocument doc(ob);

    QNetworkRequest req(QUrl(QStringLiteral("http://www.tuling123.com/openapi/api")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = nam1->post(req, doc.toJson());
    reply->setProperty("from", id);
    reply->setProperty("aiComment", aiComment);
    connect(reply, &QNetworkReply::finished, this, &Ai::receive);
}

void Ai::addTimer(int timerId, int timeOut)
{
    QTimer *timer;
    if (!timers.contains(timerId)) {
        timer = new QTimer(this);
        timer->setProperty("GraphicClient_TimerID", timerId);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, &Ai::timeout);
        timers[timerId] = timer;
    } else
        timer = timers[timerId];

    timer->setInterval(timeOut);
    timer->start();
}

void Ai::killTimer(int timerId)
{
    if (timers.contains(timerId)) {
        QTimer *timer = timers.value(timerId);
        timer->stop();
    }
}

QString Ai::getFirstChar(const QString &c)
{
    return c.left(1);
}

QString Ai::removeFirstChar(const QString &c)
{
    return c.mid(1);
}

void Ai::debugOutput(const QString &c)
{
    qDebug() << c;
}

QStringList Ai::newMessagePlayers()
{
    emit refreshPlayerList();

    QStringList r;
    QVariantMap playerDetail = AiData.value(QStringLiteral("players")).toMap();

    foreach (const QVariant &detailVariant, playerDetail) {
        QVariantMap detail = detailVariant.toMap();
        if (detail[QStringLiteral("hasUnreadMessage")].toBool())
            r.append(detail[QStringLiteral("name")].toString());
    }

    return r;
}

QStringList Ai::onlinePlayers()
{
    emit refreshPlayerList();

    QMutexLocker locker(&AiDataMutex);
    (void)locker;

    return AiData.value(QStringLiteral("players")).toMap().keys();
}

SpeakDetail Ai::getNewestSpokenMessage()
{
    SpeakDetail detail;
    detail.fromYou = false;
    detail.toYou = false;
    detail.groupSent = false;
    detail.time = 0;

    emit refreshMessageList();

    QMutexLocker locker(&AiDataMutex);
    (void)locker;

    QVariantMap message = AiData[QStringLiteral("message")].toMap();

    if (message.isEmpty())
        return detail;

    detail.from = message[QStringLiteral("from")].toString();
    detail.fromYou = message[QStringLiteral("fromYou")].toBool();
    detail.toYou = message[QStringLiteral("toYou")].toBool();
    detail.groupSent = message[QStringLiteral("groupSent")].toBool();
    detail.time = message[QStringLiteral("time")].toULongLong();
    detail.content = message[QStringLiteral("content")].toString();

    return detail;
}

void Ai::receive()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (reply == nullptr)
        return;

    QByteArray arr = reply->readAll();
    QString from = reply->property("from").toString();
    QString aiComment = reply->property("aiComment").toString();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(arr);
    QJsonObject ob = doc.object();
    int value = ob.value(QStringLiteral("code")).toInt();
    QString sending = ob.value(QStringLiteral("text")).toString();
    sending.replace(QRegExp(QStringLiteral("\\s")), QString());

    lua_getglobal(l, "tlReceive");
    lua_pushinteger(l, value);
    lua_pushstring(l, sending.toUtf8().constData());
    lua_pushstring(l, from.toUtf8().constData());
    lua_pushstring(l, aiComment.toUtf8().constData());
    pcall(4);
}

void Ai::timeout()
{
    QTimer *timer = qobject_cast<QTimer *>(sender());
    if (timer != nullptr) {
        int timerId = timer->property("GraphicClient_TimerID").toInt();
        lua_getglobal(l, "timeout");
        lua_pushinteger(l, timerId);
        pcall(1);
    }
}

void Ai::start()
{
    //    // queued connection
    //    void setNameCombo(const QString &name);
    //    void setText(const QString &text);
    //    void sendPress();
    //    void sendRelease();
    //    void sendClick();

    //    // blocking queued connection
    //    void refreshPlayerList();
    //    void refreshMessageList();

    connect(this, &Ai::setNameCombo, dialog, &Dialog::setNameCombo, Qt::QueuedConnection);
    connect(this, &Ai::setText, dialog, &Dialog::setText, Qt::QueuedConnection);
    connect(this, &Ai::sendPress, dialog, &Dialog::sendPress, Qt::QueuedConnection);
    connect(this, &Ai::sendRelease, dialog, &Dialog::sendRelease, Qt::QueuedConnection);
    connect(this, &Ai::sendClick, dialog, &Dialog::sendClick, Qt::QueuedConnection);

    connect(this, &Ai::refreshPlayerList, dialog, &Dialog::refreshPlayerList, Qt::BlockingQueuedConnection);
    connect(this, &Ai::refreshMessageList, dialog, &Dialog::refreshMessageList, Qt::BlockingQueuedConnection);

    nam1 = new QNetworkAccessManager(this);

    l = luaL_newstate();
    luaL_openlibs(l);

    QString packagePathStrFormatted = packagePathStr.arg(QDir::currentPath());
    luaL_dostring(l, packagePathStrFormatted.toUtf8().constData());

    luaopen_Ai(l);
    setMe(l, this);

    if (currentTlset.contains(QStringLiteral("aiFile"))) {
        if (luaL_dofile(l, currentTlset.value(QStringLiteral("aiFile")).toString().toLocal8Bit().constData()) != LUA_OK) {
            QString err = QString::fromUtf8(lua_tostring(l, -1));
            qDebug() << err;
            lua_pop(l, 1);
        }
    }
}
