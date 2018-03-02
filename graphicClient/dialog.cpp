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
    QFile f("config.json");
    f.open(QFile::ReadOnly);
    QByteArray arr = f.readAll();
    f.close();

    QJsonDocument doc = QJsonDocument::fromJson(arr);
    if (doc.isObject())
        config = doc.object().toVariantMap();

    if (config.contains("tlset")) {
        int tlset = config.value("tlset").toInt();

        QFile tlf("tl.json");
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
    static const QStringList l{"userName", "name", "gender"};

    foreach (const QString &i, l) {
        if (currentTlset.contains(i))
            str.append(QString("%1: %2\n").arg(i).arg(currentTlset[i].toString()));
    }

    if (currentTlset.contains("age"))
        str.append(QString("age: %1").arg(currentTlset["age"].toInt()));

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
    userNames->addItem("all");

    userNames->setMinimumWidth(QFontMetrics(qApp->font()).width('Q') * 10);
    userNames->setMaximumWidth(QFontMetrics(qApp->font()).width('Q') * 20);

    connect(userNames, &QListWidget::currentItemChanged, this, &Dialog::userNameChanged);

    sendbtn = new QPushButton("send");
    connect(sendbtn, &QPushButton::clicked, this, &Dialog::send);

    edit = new QLineEdit;
    edit->clear();
    connect(edit, &QLineEdit::returnPressed, [this]() -> void { sendbtn->animateClick(); });

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

    client = new Client(this);

    if (config.contains("serverHost")) {
        QString host = config.value("serverHost").toString();
        int port = 40001;
        if (config.contains("serverPort"))
            port = config.value("serverPort").toInt();

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
    QListWidgetItem *item = new QListWidgetItem(name);
    item->setData(GenderRole, gender);
    userNames->addItem(item);
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
        relatedPerson = "all";
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
        QString to = "all";
        QListWidgetItem *item = userNames->currentItem();
        if (item == nullptr)
            return;

        to = userNames->currentItem()->text();

        if (to == "all")
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
            playerData["name"] = item->text();
            if (item->text() != "all")
                playerData["gender"] = item->data(GenderRole).toString();
            playerData["hasUnreadMessage"] = item->data(HasUnreadMessageRole).toBool();
            playerList[item->text()] = playerData;
        }
    }

    QMutexLocker locker(&AiDataMutex);
    (void)locker;
    AiData["players"] = playerList;
}

void Dialog::refreshMessageList()
{
    QVariantMap message;
    if (listWidget->count() > 0) {
        auto item = listWidget->item(listWidget->count() - 1);
        if (item != nullptr) {
            message["from"] = item->data(FromRole).toString();
            message["fromYou"] = item->data(FromYouRole).toBool();
            message["toYou"] = item->data(ToYouRole).toBool();
            message["groupSent"] = item->data(GroupSentRole).toBool();
            message["time"] = item->data(TimeRole).toULongLong();
            message["content"] = item->data(ContentRole).toString();
        }
    }

    QMutexLocker locker(&AiDataMutex);
    (void)locker;
    AiData["message"] = message;
}

void Dialog::closeEvent(QCloseEvent *event)
{
    if (client != nullptr)
        client->disconnectFromHost();

    if (aiThread->isRunning())
        aiThread->quit();

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
            timestr = "at sometime";

        QString x = QString("%1 %2:\n%3").arg(detail.from).arg(timestr).arg(detail.content);
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
