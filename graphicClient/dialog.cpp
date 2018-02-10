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

Dialog::Dialog(QWidget *parent)
    : QWidget(parent)
{
    listWidget = new QListWidget;
    listWidget->setSortingEnabled(false);

    comboBox = new QComboBox;
    comboBox->setEditable(false);
    comboBox->addItem("all");

    comboBox->setMinimumWidth(QFontMetrics(qApp->font()).width('Q') * 15);

    sendbtn = new QPushButton("send");
    connect(sendbtn, &QPushButton::clicked, this, &Dialog::send);

    edit = new QLineEdit;
    edit->clear();
    connect(edit, &QLineEdit::returnPressed, [this]() -> void { sendbtn->animateClick(); });

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(comboBox);
    hlayout->addWidget(edit);
    hlayout->addWidget(sendbtn);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(listWidget);
    layout->addLayout(hlayout);

    setLayout(layout);

    readConfig();

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
}

void Dialog::addPlayer(QString name)
{
    comboBox->addItem(name);

    QString x = tr("Player joined: ");
    x.append(name);
    listWidget->addItem(x);
    listWidget->scrollToBottom();
}

void Dialog::removePlayer(QString name)
{
    int n = comboBox->findText(name);
    if (n != -1)
        comboBox->removeItem(n);

    QString x = tr("Player left: ");
    x.append(name);
    listWidget->addItem(x);
    listWidget->scrollToBottom();
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
    listWidget->addItem(x);
    listWidget->scrollToBottom();
}

void Dialog::send()
{
    if (!edit->text().isEmpty()) {
        QString to = comboBox->currentText();
        if (comboBox->currentText() == "all")
            to.clear();

        client->speak(to, edit->text());
        edit->clear();
    }
}

void Dialog::closeEvent(QCloseEvent *event)
{
    if (client != nullptr)
        client->disconnectFromHost();

    QWidget::closeEvent(event);
}
