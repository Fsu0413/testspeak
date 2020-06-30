#include "dialog.h"
#include "Ai.h"
#include "client.h"
#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMutex>
#include <QPushButton>
#include <QThread>
#include <QVBoxLayout>

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

Dialog::Dialog(QWidget *parent)
    : QWidget(parent)
{
    readConfig();

    listWidget = new QListWidget;
    listWidget->setSortingEnabled(false);

    userNames = new QListWidget;
    userNames->setSortingEnabled(false);
    userNames->addItem(QStringLiteral("all"));

    userNames->setMinimumWidth(QFontMetrics(qApp->font()).width(QLatin1Char('Q')) * 10);
    userNames->setMaximumWidth(QFontMetrics(qApp->font()).width(QLatin1Char('Q')) * 20);

    connect(userNames, &QListWidget::currentItemChanged, this, &Dialog::userNameChanged);

    sendbtn = new QPushButton(QStringLiteral("send"));
    connect(sendbtn, &QPushButton::clicked, this, &Dialog::send);

    edit = new AutomatedLineEdit;
    edit->clear();
    connect(edit, &AutomatedLineEdit::returnPressed, [this]() -> void { sendbtn->animateClick(); });

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(edit);
    hlayout->addWidget(sendbtn);

    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->addWidget(listWidget);
    vlayout->addLayout(hlayout);

    QVBoxLayout *vlayout2 = new QVBoxLayout;
    vlayout2->addWidget(new QLabel(generateConfigString()));
    vlayout2->addWidget(userNames);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addLayout(vlayout);
    layout->addLayout(vlayout2);

    setLayout(layout);

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
    while (aiThread->isRunning())
        aiThread->wait(1000);

    qDeleteAll(speakMap);
}

void Dialog::addPlayer(QString name, QString gender)
{
    QListWidgetItem *item = nullptr;

    for (int i = 0; i < userNames->count(); ++i) {
        if (userNames->item(i)->text() == name)
            item = userNames->item(i);
    }

    if (item == nullptr) {
        item = new QListWidgetItem(name);
        userNames->addItem(item);
    }
    item->setData(GenderRole, gender);
}

void Dialog::removePlayer(QString name)
{
    for (int i = 0; i < userNames->count(); ++i) {
        QListWidgetItem *item = userNames->item(i);
        if (item != nullptr && item->text() == name) {
            if (item->isSelected())
                userNames->setCurrentRow(-1);

            delete userNames->takeItem(i);
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
        foreach (QListWidgetItem *item, userNames->findItems(relatedPerson, Qt::MatchExactly)) {
            if (!item->isSelected()) {
                item->setBackgroundColor(qRgb(255, 0, 0));
                item->setData(HasUnreadMessageRole, true);
            }
        }
    }

    updateList();
}

void Dialog::send()
{
    if (!edit->text().isEmpty()) {
        QString to = QStringLiteral("all");
        QListWidgetItem *item = userNames->currentItem();
        if (item == nullptr)
            return;

        to = userNames->currentItem()->text();

        if (to == QStringLiteral("all"))
            to.clear();

        client->speak(to, edit->text());
        edit->clear();
    }
}

void Dialog::userNameChanged(QListWidgetItem *current, QListWidgetItem *)
{
    if (current != nullptr) {
        current->setBackgroundColor(qRgb(255, 255, 255));
        current->setData(HasUnreadMessageRole, false);
    }
    updateList();
}

void Dialog::setNameCombo(const QString &name)
{
    userNames->setFocus();
    auto items = userNames->findItems(name, Qt::MatchExactly);
    foreach (QListWidgetItem *item, items) {
        if (item->text() == name)
            userNames->setCurrentItem(item);
    }
}

void Dialog::setText(const QString &text)
{
    edit->setFocus();
    edit->deselect();
    edit->setText(text);
    edit->setCursorPosition(text.length());
}

void Dialog::setTextFocus()
{
    edit->setFocus();
    edit->selectAll();
}

void Dialog::sendPress()
{
    sendbtn->setFocus();
    sendbtn->setDown(true);
}

void Dialog::sendRelease()
{
    sendbtn->setFocus();
    sendbtn->setDown(false);
}

void Dialog::sendClick()
{
    sendbtn->setDown(false);
    sendbtn->click();
}

void Dialog::refreshPlayerList()
{
    QVariantMap playerList;
    for (int i = 0; i < userNames->count(); ++i) {
        QListWidgetItem *item = userNames->item(i);
        if (item != nullptr) {
            QVariantMap playerData;
            playerData[QStringLiteral("name")] = item->text();
            if (item->text() != QStringLiteral("all"))
                playerData[QStringLiteral("gender")] = item->data(GenderRole).toString();
            playerData[QStringLiteral("hasUnreadMessage")] = item->data(HasUnreadMessageRole).toBool();
            playerList[item->text()] = playerData;
        }
    }

    QMutexLocker locker(&AiDataMutex);
    (void)locker;
    AiData[QStringLiteral("players")] = playerList;
}

void Dialog::refreshMessageList()
{
    QVariantMap message;
    if (listWidget->count() > 0) {
        auto item = listWidget->item(listWidget->count() - 1);
        if (item != nullptr) {
            message[QStringLiteral("from")] = item->data(FromRole).toString();
            message[QStringLiteral("fromYou")] = item->data(FromYouRole).toBool();
            message[QStringLiteral("toYou")] = item->data(ToYouRole).toBool();
            message[QStringLiteral("groupSent")] = item->data(GroupSentRole).toBool();
            message[QStringLiteral("time")] = item->data(TimeRole).toULongLong();
            message[QStringLiteral("content")] = item->data(ContentRole).toString();
        }
    }

    QMutexLocker locker(&AiDataMutex);
    (void)locker;
    AiData[QStringLiteral("message")] = message;
}

void Dialog::closeEvent(QCloseEvent *event)
{
    if (aiThread->isRunning()) {
        aiThread->quit();
        while (!aiThread->isFinished())
            qApp->processEvents();
    }

    if (client != nullptr)
        client->disconnectFromHost();

    QWidget::closeEvent(event);
}

void Dialog::updateList()
{
    listWidget->clear();

    QString name;
    auto item = userNames->currentItem();
    if (item != nullptr)
        name = item->text();

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
        QListWidgetItem *item = new QListWidgetItem(x);
        item->setTextColor(detail.fromYou ? Qt::blue : Qt::magenta);

        item->setData(FromRole, detail.from);
        item->setData(FromYouRole, detail.fromYou);
        item->setData(TimeRole, qlonglong(detail.time));
        item->setData(ContentRole, detail.content);
        item->setData(ToYouRole, detail.toYou);
        item->setData(GroupSentRole, detail.groupSent);

        listWidget->addItem(item);
    }
    listWidget->scrollToBottom();
}
