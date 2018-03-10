
#include "Ai.h"
#include "client.h"
#include "lua.hpp"

#include <QApplication>
#include <QDir>
#include <QJsonArray>
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

const QString packagePathStr = "package.path = package.path .. \";%1/ai/lib/?.lua\"";

void setMe(lua_State *l, Ai *ai);

QVariantMap AiData;
QMutex AiDataMutex;

Ai::Ai(Dialog *dialog)
    : dialog(dialog)
{
}

Ai::~Ai()
{
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
    return currentTlset["name"].toString();
}

QString Ai::gender()
{
    return currentTlset["gender"].toString();
}

QString Ai::getPlayerGender(const QString &name)
{
    emit refreshPlayerList();

    QMutexLocker locker(&AiDataMutex);
    (void)locker;

    QVariantMap playerDetail = AiData.value("players").toMap().value(name).toMap();
    return playerDetail.value("gender").toString();
}

void Ai::queryTl(const QString &id, const QString &content, const QString &_key, const QString &aiComment)
{
    QJsonObject ob;
    ob["reqType"] = 0;

    QString key = _key;
    if (_key.isEmpty())
        key = currentTlset["key"].toString();

    QJsonObject userInfo;
    userInfo["apiKey"] = QJsonValue(key);
    userInfo["userId"] = QJsonValue(id);
    ob["userInfo"] = userInfo;

    QJsonObject perception;
    QJsonObject inputText;
    inputText["text"] = content;
    perception["inputText"] = inputText;
    ob["perception"] = perception;

    QJsonDocument doc(ob);

    QNetworkRequest req(QUrl(QStringLiteral("http://openapi.tuling123.com/openapi/api/v2")));
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
    QVariantMap playerDetail = AiData.value("players").toMap();

    foreach (const QVariant &detailVariant, playerDetail) {
        QVariantMap detail = detailVariant.toMap();
        if (detail["hasUnreadMessage"].toBool())
            r.append(detail["name"].toString());
    }

    return r;
}

QStringList Ai::onlinePlayers()
{
    emit refreshPlayerList();

    QMutexLocker locker(&AiDataMutex);
    (void)locker;

    return AiData.value("players").toMap().keys();
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

    QVariantMap message = AiData["message"].toMap();

    if (message.isEmpty())
        return detail;

    detail.from = message["from"].toString();
    detail.fromYou = message["fromYou"].toBool();
    detail.toYou = message["toYou"].toBool();
    detail.groupSent = message["groupSent"].toBool();
    detail.time = message["time"].toULongLong();
    detail.content = message["content"].toString();

    return detail;
}

void Ai::convertJsonValueToLuaValue(const QJsonValue &value)
{
    switch (value.type()) {
    case QJsonValue::Null:
        break;
    case QJsonValue::Bool:
        lua_pushboolean(l, value.toBool());
        break;
    case QJsonValue::Double: {
        double d = value.toDouble();
        int i = value.toInt();
        if (qFuzzyCompare(d, (double)i))
            lua_pushinteger(l, i);
        else
            lua_pushnumber(l, d);
    } break;
    case QJsonValue::String:
        lua_pushstring(l, value.toString().toUtf8().constData());
        break;
    case QJsonValue::Array: {
        QJsonArray arr = value.toArray();
        lua_createtable(l, arr.size(), 0);
        int i = 0;
        foreach (const QJsonValue &v, arr) {
            if (v.isNull())
                continue;
            lua_pushinteger(l, ++i);
            convertJsonValueToLuaValue(v);
            lua_settable(l, -3);
        }
    } break;
    case QJsonValue::Object: {
        QJsonObject ob = value.toObject();
        lua_createtable(l, 0, ob.size());

        foreach (const QString &key, ob.keys()) {
            QJsonValue v = ob.value(key);
            if (v.isNull())
                continue;

            convertJsonValueToLuaValue(v);
            lua_setfield(l, -2, key.toUtf8().constData());
        }
    } break;
    default:
        // ??
        break;
    }
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
    qDebug() << doc.toJson(QJsonDocument::Indented);
    QJsonObject ob = doc.object();

    lua_getglobal(l, "tlReceive");
    convertJsonValueToLuaValue(QJsonValue(ob));
    lua_pushstring(l, from.toUtf8().constData());
    lua_pushstring(l, aiComment.toUtf8().constData());
    pcall(3);
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

    if (currentTlset.contains("aiFile")) {
        if (luaL_dofile(l, currentTlset.value("aiFile").toString().toLocal8Bit().constData()) != LUA_OK) {
            QString err = QString::fromUtf8(lua_tostring(l, -1));
            qDebug() << err;
            lua_pop(l, 1);
        }
    }
}
