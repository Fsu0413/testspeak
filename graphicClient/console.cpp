#include "Ai.h"
#include "client.h"
#include "dialog.h"
#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMap>
#include <QMutex>
#include <QThread>

#include <iostream>

using namespace std;

QVariantMap config;
QVariantMap currentTlset;

void readConfig()
{
    QFile f(QStringLiteral("config.json"));
    f.open(QFile::ReadOnly);
    QByteArray arr = f.readAll();
    f.close();

    QJsonDocument doc = QJsonDocument::fromJson(arr);
    if (doc.isObject())
        config = doc.object().toVariantMap();

    if (config.contains(QStringLiteral("tlset"))) {
        int tlset = config.value(QStringLiteral("tlset")).toInt();

        QFile tlf(QStringLiteral("tl.json"));
        tlf.open(QFile::ReadOnly);
        QByteArray tlarr = tlf.readAll();
        tlf.close();

        QJsonParseError err;
        QJsonDocument tldoc = QJsonDocument::fromJson(tlarr, &err);
        currentTlset = tldoc.array().at(tlset).toObject().toVariantMap();
    }
}

QString generateConfigString()
{
    QString str;
    static const QStringList l {QStringLiteral("userName"), QStringLiteral("name"), QStringLiteral("gender")};

    foreach (const QString &i, l) {
        if (currentTlset.contains(i))
            str.append(QStringLiteral("%1: %2\n").arg(i).arg(currentTlset[i].toString()));
    }

    if (currentTlset.contains(QStringLiteral("age")))
        str.append(QStringLiteral("age: %1").arg(currentTlset[QStringLiteral("age")].toInt()));

    str = str.trimmed();
    return str;
}

Dialog::Dialog(QObject *parent)
    : QObject(parent)
{
    readConfig();

    qDebug() << generateConfigString().split(QStringLiteral("\n"));

    PlayerDetail *detail = new PlayerDetail;
    detail->name = QStringLiteral("all");
    detail->hasUnreadMessage = false;
    playerMap[QStringLiteral("all")] = detail;

    Ai *ai = new Ai(this);
    aiThread = new QThread(this);
    ai->moveToThread(aiThread);
    connect(aiThread, &QThread::finished, ai, &Ai::deleteLater);
    connect(aiThread, &QThread::started, ai, &Ai::start, Qt::QueuedConnection);

    client = new Client(this);

    connect(client, &Client::addPlayer, this, &Dialog::addPlayer);
    connect(client, &Client::removePlayer, this, &Dialog::removePlayer);
    connect(client, &Client::playerDetail, this, &Dialog::playerDetail);
    connect(client, &Client::playerSpoken, this, &Dialog::playerSpoken);
    connect(client, &Client::signedIn, [this]() { aiThread->start(); });

    if (config.contains(QStringLiteral("serverHost"))) {
        QString host = config.value(QStringLiteral("serverHost")).toString();
        int port = 40001;
        if (config.contains(QStringLiteral("serverPort")))
            port = config.value(QStringLiteral("serverPort")).toInt();

        client->connectToHost(host, port);
    }
}

Dialog::~Dialog()
{
    while (!aiThread->isFinished())
        aiThread->wait(1000);

    qDeleteAll(speakMap);
}

void Dialog::addPlayer(QString name, QString gender)
{
    PlayerDetail *detail = new PlayerDetail;
    detail->name = name;
    detail->gender = gender;
    detail->hasUnreadMessage = false;
    playerMap[name] = detail;
}

void Dialog::removePlayer(QString name)
{
    if (speakTo == name) {
        speakTo.clear();
        userNameChanged();
    }

    if (playerMap.contains(name))
        delete playerMap.take(name);

    if (speakMap.contains(name))
        delete speakMap.take(name);
}

void Dialog::playerDetail(QJsonObject)
{
    // do nothing
}

void Dialog::playerSpoken(QString from, QString to, QString content, bool fromYou, bool toYou, bool groupsent, quint32 time)
{
    QString dbg = QStringLiteral("Said: %1 to %2: %3").arg(from).arg(to).arg(content);
    std::cout << dbg.toLocal8Bit().constData() << std::endl;

    SpeakDetail x;
    x.content = content;
    x.from = from;
    x.fromYou = fromYou;
    x.toYou = toYou;
    x.groupSent = groupsent;
    x.time = time;

    QString relatedPerson;
    if (groupsent)
        relatedPerson = QStringLiteral("all");
    else if (fromYou)
        relatedPerson = to;
    else
        relatedPerson = from;

    if (!speakMap.contains(relatedPerson))
        speakMap[relatedPerson] = new QList<SpeakDetail>;

    speakMap[relatedPerson]->append(x);
    if (speakMap[relatedPerson]->length() > 100)
        speakMap[relatedPerson]->takeFirst();

    if (!fromYou && playerMap.contains(relatedPerson) && speakTo != relatedPerson)
        playerMap[relatedPerson]->hasUnreadMessage = true;
}

void Dialog::send()
{
    if (!edit.isEmpty()) {
        QString to = QStringLiteral("all");

        if (speakTo.isNull())
            return;

        to = speakTo;

            QString dbg = QStringLiteral("Send: %1: %2").arg(to).arg(edit);
            std::cout << dbg.toLocal8Bit().constData() << std::endl;

        if (to == QStringLiteral("all"))
            to.clear();

        client->speak(to, edit);
        edit.clear();
    }
}

void Dialog::userNameChanged()
{
    if (playerMap.contains(speakTo))
        playerMap[speakTo]->hasUnreadMessage = false;
}

void Dialog::setNameCombo(const QString &name)
{
    if (playerMap.contains(name)) {
        if (speakTo != name)
            {
                QString dbg = QStringLiteral("Changing current speak to %1").arg(name);
                std::cout << dbg.toLocal8Bit().constData() << std::endl;
            }
        speakTo = name;
        userNameChanged();
    }
}

void Dialog::setText(const QString &text)
{
    QString outputText = QStringLiteral("\n") + text;
    if (text.startsWith(edit))
        outputText = text.mid(edit.length());
    std::cout << outputText.toLocal8Bit().constData() << std::flush;
    edit = text;
}

void Dialog::setTextFocus()
{
}

void Dialog::sendPress()
{
    QString outputText = QStringLiteral("\u21B2");
std::cout << outputText.toLocal8Bit().constData() << std::flush;

}

void Dialog::sendRelease()
{
}

void Dialog::sendClick()
{
    std::cout << std::endl;
    send();
}

void Dialog::refreshPlayerList()
{
    QVariantMap playerList;
    foreach (PlayerDetail *detail, playerMap) {
        QVariantMap playerData;
        playerData[QStringLiteral("name")] = detail->name;
        if (detail->name != QStringLiteral("all"))
            playerData[QStringLiteral("gender")] = detail->gender;
        playerData[QStringLiteral("hasUnreadMessage")] = detail->hasUnreadMessage;
        playerList[detail->name] = playerData;
    }

    QMutexLocker locker(&AiDataMutex);
    (void)locker;
    AiData[QStringLiteral("players")] = playerList;
}

void Dialog::refreshMessageList()
{
    QVariantMap message;
    QString name = speakTo;

    if (speakMap.contains(name)) {
        QList<SpeakDetail> *details = speakMap[name];
        if (details->length() > 0) {
            const SpeakDetail &detail = details->last();

            message[QStringLiteral("from")] = detail.from;
            message[QStringLiteral("fromYou")] = detail.fromYou;
            message[QStringLiteral("toYou")] = detail.toYou;
            message[QStringLiteral("groupSent")] = detail.groupSent;
            message[QStringLiteral("time")] = qlonglong(detail.time);
            message[QStringLiteral("content")] = detail.content;
        }
    }

    QMutexLocker locker(&AiDataMutex);
    (void)locker;
    AiData[QStringLiteral("message")] = message;
}
