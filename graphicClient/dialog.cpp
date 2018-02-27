#include "dialog.h"
#include "Ai.h"
#include "client.h"
#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
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
#include <QPushButton>
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

    new Ai(client, this);
}

Dialog::~Dialog()
{
    qDeleteAll(speakMap);
}

void Dialog::addPlayer(QString name)
{
    userNames->addItem(name);
}

void Dialog::removePlayer(QString name)
{
    for (int i = 0; i < userNames->count(); ++i) {
        QListWidgetItem *item = userNames->item(i);
        if (item != nullptr && item->text() == name) {
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
    if (fromYou)
        from = tr("you");

    if (toYou)
        to = tr("you");
    else if (groupsent)
        to = tr("__ALLPLAYERS__");

    QString timestr;
    if (time != 0)
        timestr = QDateTime::fromTime_t(time).time().toString();
    else
        timestr = "sometime";

    QString x = tr("%1 said to %2 at %3: ").arg(from).arg(to).arg(timestr);
    x.append(content);

    QString relatedPerson;
    if (groupsent)
        relatedPerson = "all";
    else if (fromYou)
        relatedPerson = to;
    else
        relatedPerson = from;

    if (!speakMap.contains(relatedPerson))
        speakMap[relatedPerson] = new QStringList;

    speakMap[relatedPerson]->append(x);
    if (speakMap[relatedPerson]->length() > 100)
        speakMap[relatedPerson]->takeFirst();

    foreach (QListWidgetItem *item, userNames->findItems(relatedPerson, Qt::MatchExactly)) {
        if (!item->isSelected())
            item->setBackgroundColor(qRgb(255, 0, 0));
    }

    updateList();
}

void Dialog::send()
{
    if (!edit->text().isEmpty()) {
        QString to = "all";
        QListWidgetItem *item = userNames->currentItem();
        if (item != nullptr)
            to = userNames->currentItem()->text();

        if (to == "all")
            to.clear();

        client->speak(to, edit->text());
        edit->clear();
    }
}

void Dialog::userNameChanged(QListWidgetItem *current, QListWidgetItem *)
{
    current->setBackgroundColor(qRgb(255, 255, 255));
    QString currentname = current->text();
    updateList();
}

void Dialog::closeEvent(QCloseEvent *event)
{
    if (client != nullptr)
        client->disconnectFromHost();

    QWidget::closeEvent(event);
}

void Dialog::updateList()
{
    QString name;
    auto item = userNames->currentItem();
    if (item != nullptr)
        name = item->text();

    if (name.isEmpty())
        return;

    listWidget->clear();
    if (!speakMap.contains(name))
        return;

    listWidget->addItems(*(speakMap[name]));
    listWidget->scrollToBottom();
}
