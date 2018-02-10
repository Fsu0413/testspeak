
#include "Ai.h"
#include "client.h"
#include "dialog.h"
#include "lua.hpp"
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLineEdit>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QTimer>
#include <QUrl>

extern QVariantMap config;
extern QVariantMap currentTlset;

extern "C" {
void luaopen_Ai(lua_State *l);
}

void setMe(lua_State *l, Ai *ai);

Ai::Ai(Client *client, Dialog *parent)
    : QObject(parent)
    , dialog(parent)
    , client(client)
{
    nam1 = new QNetworkAccessManager(this);

    connect(client, &Client::addPlayer, this, &Ai::addPlayer);
    connect(client, &Client::removePlayer, this, &Ai::removePlayer);
    connect(client, &Client::playerDetail, this, &Ai::playerDetail);
    connect(client, &Client::playerSpoken, this, &Ai::playerSpoken);

    l = luaL_newstate();
    luaL_openlibs(l);
    luaopen_Ai(l);
    setMe(l, this);

    QTimer::singleShot(1, [this]() -> void {
        if (currentTlset.contains("aiFile")) {
            if (luaL_dofile(l, currentTlset.value("aiFile").toString().toLocal8Bit().constData()) != LUA_OK) {
                QString err = QString::fromUtf8(lua_tostring(l, -1));
                qDebug() << err;
                lua_pop(l, 1);
            }
        }
    });
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

void Ai::queryPlayer(const QString &name)
{
    client->queryPlayerDetail(name);
}

void Ai::queryTl(const QString &id, const QString &content)
{
    QJsonObject ob;

    ob["key"] = QJsonValue(currentTlset["key"].toString());
    ob["userid"] = QJsonValue(id);
    ob["info"] = QJsonValue(content);

    QJsonDocument doc(ob);

    QNetworkRequest req(QUrl(QStringLiteral("http://www.tuling123.com/openapi/api")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = nam1->post(req, doc.toJson());
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

bool Ai::setNameCombo(const QString &name)
{
    int n = dialog->comboBox->findText(name);
    if (n != -1 && n != dialog->comboBox->currentIndex()) {
        dialog->comboBox->setCurrentIndex(n);
        return true;
    }

    return false;
}

void Ai::setText(const QString &text)
{
    dialog->edit->setFocus();
    dialog->edit->deselect();
    dialog->edit->setText(text);
    dialog->edit->setCursorPosition(text.length());
}

void Ai::animateSend(int timeout)
{
    dialog->sendbtn->animateClick(timeout);
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

void Ai::addPlayer(QString name)
{
    lua_getglobal(l, "addPlayer");
    lua_pushstring(l, name.toUtf8().constData());
    pcall(1);
}

void Ai::removePlayer(QString name)
{
    lua_getglobal(l, "removePlayer");
    lua_pushstring(l, name.toUtf8().constData());
    pcall(1);
}

void Ai::playerDetail(QJsonObject ob)
{
    QString obName = ob.value("name").toString();
    QString obGender = ob.value("gender").toString();

    lua_getglobal(l, "playerDetail");
    lua_pushstring(l, obName.toUtf8().constData());
    lua_pushstring(l, obGender.toUtf8().constData());
    pcall(2);
}

void Ai::playerSpoken(QString from, QString to, QString content, bool fromYou, bool toYou, bool groupsent)
{
    lua_getglobal(l, "playerSpoken");
    lua_pushstring(l, from.toUtf8().constData());
    lua_pushstring(l, to.toUtf8().constData());
    lua_pushstring(l, content.toUtf8().constData());
    lua_pushboolean(l, fromYou);
    lua_pushboolean(l, toYou);
    lua_pushboolean(l, groupsent);
    pcall(6);
}

void Ai::receive()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (reply == nullptr)
        return;

    QByteArray arr = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(arr);
    QJsonObject ob = doc.object();
    int value = ob.value(QStringLiteral("code")).toInt();
    QString sending = ob.value("text").toString();
    sending.replace(QRegExp("\\s"), QString());

    lua_getglobal(l, "tlReceive");
    lua_pushinteger(l, value);
    lua_pushstring(l, sending.toUtf8().constData());
    pcall(2);
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
