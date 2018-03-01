
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

const QString packagePathStr = "package.path = package.path .. \";%1/ai/lib/?.lua\"";

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

    QString packagePathStrFormatted = packagePathStr.arg(QDir::currentPath());
    luaL_dostring(l, packagePathStrFormatted.toUtf8().constData());

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

void Ai::queryTl(const QString &id, const QString &content, const QString &_key, const QString &aiComment)
{
    QJsonObject ob;

    QString key = _key;
    if (_key.isEmpty())
        key = currentTlset["key"].toString();

    ob["key"] = QJsonValue(key);
    ob["userid"] = QJsonValue(id);
    ob["info"] = QJsonValue(content);

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

bool Ai::setNameCombo(const QString &name)
{
    dialog->userNames->setFocus();
    auto items = dialog->userNames->findItems(name, Qt::MatchExactly);
    foreach (QListWidgetItem *item, items) {
        if (item->text() == name) {
            dialog->userNames->setCurrentItem(item);
            return true;
        }
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

void Ai::sendPress()
{
    dialog->sendbtn->setFocus();
    dialog->sendbtn->setDown(true);
}

void Ai::sendRelease()
{
    dialog->sendbtn->setFocus();
    dialog->sendbtn->setDown(false);
}

void Ai::sendClick()
{
    dialog->sendbtn->setDown(false);
    dialog->sendbtn->click();
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

void Ai::prepareExit()
{
    qApp->exit();
}

QString Ai::firstUnreadMessageFrom()
{
    for (int i = 0; i < dialog->userNames->count(); ++i) {
        auto item = dialog->userNames->item(i);
        if (item != nullptr) {
            if (item->data(HasUnreadMessageRole).toBool())
                return item->text();
        }
    }

    return QString();
}

QStringList Ai::newMessagePlayers()
{
    QStringList r;
    for (int i = 0; i < dialog->userNames->count(); ++i) {
        auto item = dialog->userNames->item(i);
        if (item != nullptr) {
            if (item->data(HasUnreadMessageRole).toBool())
                r.append(item->text());
        }
    }

    return r;
}

QStringList Ai::onlinePlayers()
{
    QStringList r;
    for (int i = 0; i < dialog->userNames->count(); ++i) {
        auto item = dialog->userNames->item(i);
        if (item != nullptr) {
            // if (item->text() != "all")
            r.append(item->text());
        }
    }

    return r;
}

SpeakDetail Ai::getNewestSpokenMessage()
{
    SpeakDetail detail;
    detail.fromYou = false;
    detail.toYou = false;
    detail.groupSent = false;
    detail.time = 0;

    if (dialog->listWidget->count() == 0)
        return detail;

    auto item = dialog->listWidget->item(dialog->listWidget->count() - 1);
    if (item == nullptr)
        return detail;

    detail.from = item->data(FromRole).toString();
    detail.fromYou = item->data(FromYouRole).toBool();
    detail.toYou = item->data(ToYouRole).toBool();
    detail.groupSent = item->data(GroupSentRole).toBool();
    detail.time = item->data(TimeRole).toULongLong();
    detail.content = item->data(ContentRole).toString();

    return detail;
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
    QString obName = ob.value("userName").toString();
    QString obGender = ob.value("gender").toString();

    lua_getglobal(l, "playerDetail");
    lua_pushstring(l, obName.toUtf8().constData());
    lua_pushstring(l, obGender.toUtf8().constData());
    pcall(2);
}

void Ai::playerSpoken(QString from, QString to, QString content, bool fromYou, bool toYou, bool groupsent, quint32 time)
{
    lua_getglobal(l, "playerSpoken");
    lua_pushstring(l, from.toUtf8().constData());
    lua_pushstring(l, to.toUtf8().constData());
    lua_pushstring(l, content.toUtf8().constData());
    lua_pushboolean(l, fromYou);
    lua_pushboolean(l, toYou);
    lua_pushboolean(l, groupsent);
    lua_pushinteger(l, (lua_Integer)time);
    pcall(7);
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
    QString sending = ob.value("text").toString();
    sending.replace(QRegExp("\\s"), QString());

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
