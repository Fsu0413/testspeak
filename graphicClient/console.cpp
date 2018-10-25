#include "console.h"
#include "Ai.h"
#include "client.h"
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMap>
#include <QMutex>
#include <QThread>

struct ConsoleListItem
{
    QString text;
    QMap<int, QVariant> data;
};

class ConsoleList : public QObject
{
    Q_OBJECT

public:
    ~ConsoleList();

    QList<ConsoleListItem *> item;
    int selected;

signals:
    void selectedChanged();
};

class ConsoleButton : public QObject
{
    Q_OBJECT

signals:
    void clicked();
};

ConsoleList::~ConsoleList()
{
    qDeleteAll(item);
}

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

    listWidget = new ConsoleList;
    userNames = new ConsoleList;
    userNames->selected = -1;
    ConsoleListItem *allItem = new ConsoleListItem;
    allItem->text = QStringLiteral("all");
    userNames->item << allItem;

    connect(userNames, &ConsoleList::selectedChanged, this, &Dialog::userNameChanged);

    sendBtn = new ConsoleButton;
    connect(sendBtn, &ConsoleButton::clicked, this, &Dialog::send);

    client = new Client(this);

    if (config.contains(QStringLiteral("serverHost"))) {
        QString host = config.value(QStringLiteral("serverHost")).toString();
        int port = 40001;
        if (config.contains(QStringLiteral("serverPort")))
            port = config.value(QStringLiteral("serverPort")).toInt();

        client->connectToHost(host, port);
    }

    connect(client, &Client::addPlayer, this, &Dialog::addPlayer);
    connect(client, &Client::removePlayer, this, &Dialog::removePlayer);
    connect(client, &Client::playerDetail, this, &Dialog::playerDetail);
    connect(client, &Client::playerSpoken, this, &Dialog::playerSpoken);

    Ai *ai = new Ai(this);
    aiThread = new QThread(this);
    ai->moveToThread(aiThread);
    connect(aiThread, &QThread::finished, ai, &Ai::deleteLater);
    connect(aiThread, &QThread::started, ai, &Ai::start);
    aiThread->start();
}

Dialog::~Dialog()
{
    while (!aiThread->isFinished()) {
        qDebug() << "wait";
        aiThread->wait(1000);
    }

    qDeleteAll(speakMap);
}

void Dialog::addPlayer(QString name, QString gender)
{
    ConsoleListItem *allItem = new ConsoleListItem;
    allItem->text = name;
    allItem->data[GenderRole] = gender;
    userNames->item << allItem;
}

void Dialog::removePlayer(QString name)
{
    for (int i = 0; i < userNames->item.length(); ++i) {
        ConsoleListItem *item = userNames->item[i];
        if (item->text == name) {
            if (userNames->selected == i) {
                userNames->selected = -1;
                emit userNames->selectedChanged();
            }

            delete userNames->item.takeAt(i);
            i = -1;
        }
    }

    if (speakMap.contains(name))
        delete speakMap.take(name);
}

void Dialog::playerDetail(QJsonObject)
{
    // do nothing
}

void Dialog::playerSpoken(QString from, QString to, QString content, bool fromYou, bool toYou, bool groupsent, quint32 time)
{
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

    if (!fromYou) {
        foreach (ConsoleListItem *item, userNames->item) {
            if (item->text == relatedPerson)
                item->data[HasUnreadMessageRole] = true;
        }
    }

    updateList();
}

void Dialog::send()
{
    if (!edit.isEmpty()) {
        QString to = QStringLiteral("all");
        if (userNames->selected == -1)
            return;

        to = userNames->item[userNames->selected]->text;

        if (to == QStringLiteral("all"))
            to.clear();

        client->speak(to, edit);
        edit.clear();
    }
}

void Dialog::userNameChanged()
{
    if (userNames->selected != -1)
        userNames->item[userNames->selected]->data[HasUnreadMessageRole] = false;

    updateList();
}

void Dialog::setNameCombo(const QString &name)
{
    for (int i = 0; i < userNames->item.length(); ++i) {
        if (userNames->item[i]->text == name) {
            userNames->selected = i;
            emit userNames->selectedChanged();
        }
    }
}

void Dialog::setText(const QString &text)
{
    edit = text;
}

void Dialog::sendPress()
{
}

void Dialog::sendRelease()
{
}

void Dialog::sendClick()
{
    emit sendBtn->clicked();
}

void Dialog::refreshPlayerList()
{
    QVariantMap playerList;
    for (int i = 0; i < userNames->item.length(); ++i) {
        ConsoleListItem *item = userNames->item[i];
        QVariantMap playerData;
        playerData[QStringLiteral("name")] = item->text;
        if (item->text != QStringLiteral("all"))
            playerData[QStringLiteral("gender")] = item->data[GenderRole].toString();
        playerData[QStringLiteral("hasUnreadMessage")] = item->data[HasUnreadMessageRole].toBool();
        playerList[item->text] = playerData;
    }

    QMutexLocker locker(&AiDataMutex);
    (void)locker;
    AiData[QStringLiteral("players")] = playerList;
}

void Dialog::refreshMessageList()
{
    QVariantMap message;

    if (listWidget->item.length() > 0) {
        ConsoleListItem *item = listWidget->item.last();
        message[QStringLiteral("from")] = item->data[FromRole].toString();
        message[QStringLiteral("fromYou")] = item->data[FromYouRole].toBool();
        message[QStringLiteral("toYou")] = item->data[ToYouRole].toBool();
        message[QStringLiteral("groupSent")] = item->data[GroupSentRole].toBool();
        message[QStringLiteral("time")] = item->data[TimeRole].toULongLong();
        message[QStringLiteral("content")] = item->data[ContentRole].toString();
    }

    QMutexLocker locker(&AiDataMutex);
    (void)locker;
    AiData[QStringLiteral("message")] = message;
}

void Dialog::updateList()
{
    qDeleteAll(listWidget->item);

    QString name;
    if (userNames->selected != -1)
        name = userNames->item[userNames->selected]->text;

    if (name.isEmpty())
        return;

    if (!speakMap.contains(name))
        return;

    QList<SpeakDetail> *details = speakMap[name];
    foreach (const SpeakDetail &detail, *details) {
        QString timestr;
        if (detail.time != 0)
            timestr = QDateTime::fromTime_t(detail.time).time().toString();
        else
            timestr = QStringLiteral("at sometime");

        QString x = QStringLiteral("%1 %2:\n%3").arg(detail.from).arg(timestr).arg(detail.content);

        ConsoleListItem *item = new ConsoleListItem;
        item->text = x;

        item->data[FromRole] = detail.from;
        item->data[FromYouRole] = detail.fromYou;
        item->data[TimeRole] = qlonglong(detail.time);
        item->data[ContentRole] = detail.content;
        item->data[ToYouRole] = detail.toYou;
        item->data[GroupSentRole] = detail.groupSent;

        listWidget->item << item;
    }
}

#include "console.moc"
