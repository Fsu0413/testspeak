#include "dialog.h"
#include "client.h"

#include <QCloseEvent>
#include <QDebug>
#include <QFile>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpServer>
#include <QTimer>
#include <QVBoxLayout>
#include <QtAlgorithms>
#include <QtMath>
#include <functional>
#include <random>

const int thinkdelay = 3000; //1000;
const int clickDelay = 300;
const int typedelay = 100;
const int sendDelay = 500;
const int timeoutinterval = 150000;

std::random_device rd;

inline int generateRandom(int in)
{
    int min = in * 0.3;
    int max = in * 1.5;
    int diff = max - min;

    return (rd() % diff) + min;
}

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

class Ai : public QObject
{
    Q_OBJECT
public:
    Dialog *dialog;
    Client *client;

    QString speakingTo;
    QStringList groupSpoken;
    QMap<QString, QString> spokenToMe;

    QString lastSent;
    QString lastRecv;

    QNetworkAccessManager *nam1;
    QTimer *findPersonTimer;

    QTimer *timeoutTimer;
    int timeoutTime;

    QList<std::pair<int, std::function<void()>>> operateGuiList;

    Ai(Client *client, Dialog *parent)
        : QObject(parent)
        , client(client)
        , timeoutTime(0)
    {
        nam1 = new QNetworkAccessManager(this);
        findPersonTimer = new QTimer(this);

        connect(client, &Client::addPlayer, this, &Ai::addPlayer);
        connect(client, &Client::removePlayer, this, &Ai::removePlayer);
        connect(client, &Client::playerDetail, this, &Ai::playerDetail);
        connect(client, &Client::playerSpoken, this, &Ai::playerSpoken);

        findPersonTimer->setInterval(60000);
        findPersonTimer->setSingleShot(false);
        connect(findPersonTimer, &QTimer::timeout, this, &Ai::findPerson);
        findPersonTimer->start();

        QTimer::singleShot(100, this, SLOT(operateGui()));
        QTimer::singleShot(100, this, SLOT(findPerson()));

        timeoutTimer = new QTimer(this);
        timeoutTimer->setSingleShot(true);
        connect(timeoutTimer, &QTimer::timeout, this, &Ai::timeout);
    }

    void typeString(QString text)
    {
        Dialog *dialog = qobject_cast<Dialog *>(parent());
        if (dialog == nullptr)
            return;

        dialog->edit->setFocus();
        dialog->edit->deselect();
        dialog->edit->setText(text);
        dialog->edit->setCursorPosition(text.length());
    }

    void clientSpeak(QString to, QString toSend)
    {
        Dialog *dialog = qobject_cast<Dialog *>(parent());
        if (dialog == nullptr)
            return;

        QList<std::pair<int, std::function<void()>>> list;

        if (to.isEmpty())
            to = "all";

        int n = dialog->comboBox->findText(to);
        if (n != -1 && n != dialog->comboBox->currentIndex()) {
            list.append(std::make_pair<int, std::function<void()>>(generateRandom(clickDelay), [dialog]() -> void { dialog->comboBox->showPopup(); }));
            list.append(std::make_pair<int, std::function<void()>>(generateRandom(clickDelay), [dialog, n]() -> void {
                dialog->comboBox->setCurrentIndex(n);
                dialog->comboBox->hidePopup();
            }));
        }

        list.append(std::make_pair<int, std::function<void()>>(generateRandom(thinkdelay), []() -> void {}));

        QString text;

        while (!toSend.isEmpty()) {
            QChar c = toSend.left(1).at(0);
            text.append(c);
            toSend = toSend.mid(1);
            list.append(std::make_pair<int, std::function<void()>>(generateRandom(typedelay), std::bind(&Ai::typeString, this, text)));
        }

        list.append(std::make_pair<int, std::function<void()>>(generateRandom(sendDelay),
                                                               [dialog, this, to]() -> void {
                                                                   if (to == dialog->comboBox->currentText())
                                                                       dialog->sendbtn->animateClick(generateRandom(clickDelay));
                                                                   else
                                                                       dialog->edit->clear();
                                                               }

                                                               ));

        list.append(operateGuiList);
        operateGuiList = list;
    }

    QString getStringFromBase(const QString &key)
    {
        QFile f("base.json");
        f.open(QFile::ReadOnly);
        QByteArray arr = f.readAll();
        f.close();

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &err);

        QJsonObject ob = doc.object();
        QJsonArray obarr = ob[key].toArray();
        int size = obarr.size();
        if (size == 0) {
            return "hello";
        } else {
            QJsonValue v = obarr.at(rd() % size);
            return v.toString();
        }
    }

    void send(const QString &sending)
    {
        if (!speakingTo.isEmpty()) {
            lastSent = sending;
            operateGuiList.append(std::make_pair<int, std::function<void()>>(generateRandom(clickDelay), std::bind(&Ai::clientSpeak, this, speakingTo, sending)));
            timeoutTimer->start(generateRandom(timeoutinterval));
        }
    }

    void talk(const QString &content)
    {
        if (content == lastRecv) {
            QString sending = getStringFromBase("recvdup");
            send(sending);
        } else {
            lastRecv = content;
            analyzeContent();
        }
    }

    void analyzeContent()
    {
        if (lastRecv == lastSent) {
            QString sending = getStringFromBase("parrotdup");
            send(sending);
            return;
        }

        QJsonObject ob;

        ob["key"] = QJsonValue(currentTlset["key"].toString());
        ob["userid"] = QJsonValue(speakingTo);
        ob["info"] = QJsonValue(lastRecv);

        QJsonDocument doc(ob);

        QNetworkRequest req(QUrl(QStringLiteral("http://www.tuling123.com/openapi/api")));
        req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("text/json"));

        QNetworkReply *reply = nam1->post(req, doc.toJson());
        connect(reply, &QNetworkReply::finished, this, &Ai::receive);
    }

    void receive()
    {
        QString sending;
        QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
        if (reply == nullptr)
            return;

        QByteArray arr = reply->readAll();
        reply->deleteLater();
        QJsonDocument doc = QJsonDocument::fromJson(arr);

        QJsonObject ob = doc.object();
        if (ob.contains(QStringLiteral("code"))) {
            QJsonValue value = ob.value(QStringLiteral("code"));
            int x = value.toInt();
            switch (x) {
            case 100000: {
                sending = ob.value("text").toString();
                sending.replace(QRegExp("\\s"), QString());
                if (sending == lastSent)
                    sending = getStringFromBase("senddup");
                break;
            }
            case 40004:
                qDebug() << x;
                return;
            default:
                qDebug() << x;
            }
        }

        if (sending.isEmpty())
            sending = getStringFromBase("change");

        send(sending);
    }

    void timeout()
    {
        QString sending = getStringFromBase(QString("timeout%1").arg(++timeoutTime));
        send(sending);

        if (timeoutTime >= 3) {
            speakingTo.clear();
        } else {
            timeoutTimer->start(generateRandom(timeoutinterval));
        }
    }

public slots:
    void addPlayer(QString)
    {
        // donothing
    }

    void removePlayer(QString name)
    {
        if (speakingTo == name) {
            speakingTo.clear();
            timeoutTimer->stop();
        }
    }

    void playerDetail(QJsonObject ob)
    {
        QString obName = ob.value("name").toString();
        QString obGender = ob.value("gender").toString();
        QString gender = currentTlset.value("gender").toString();

        if (spokenToMe.contains(obName) && speakingTo.isEmpty()) {
            if (gender != obGender) {
                speakingTo = obName;
                talk(spokenToMe.value(obName));
                spokenToMe.clear();
                groupSpoken.clear();
            } else {
                QString sending = getStringFromBase(QString("g") + gender);
                operateGuiList.append(std::make_pair<int, std::function<void()>>(generateRandom(clickDelay), std::bind(&Ai::clientSpeak, this, obName, sending)));
                spokenToMe.remove(obName);
            }
        } else if (groupSpoken.contains(obName) && speakingTo.isEmpty()) {
            if (gender != obGender) {
                QString sending = getStringFromBase("greet");
                sending = sending.arg(currentTlset.value("name").toString());
                operateGuiList.append(std::make_pair<int, std::function<void()>>(generateRandom(clickDelay), std::bind(&Ai::clientSpeak, this, obName, sending)));
            } else
                groupSpoken.removeAll(obName);
        }
    }

    void playerSpoken(QString from, QString, QString content, bool fromYou, bool toYou, bool groupsent)
    {
        if (fromYou)
            return;

        if (groupsent && speakingTo.isEmpty()) {
            groupSpoken << from;
            client->queryPlayerDetail(from);
        }

        if (toYou && speakingTo.isEmpty()) {
            spokenToMe[from] = content;
            client->queryPlayerDetail(from);
        }

        if (toYou && from == speakingTo) {
            timeoutTimer->stop();
            timeoutTime = 0;
            talk(content);
        }
    }

    void findPerson()
    {
        if (speakingTo.isEmpty()) {
            QString sending = getStringFromBase("findperson");
            operateGuiList.append(std::make_pair<int, std::function<void()>>(generateRandom(clickDelay), std::bind(&Ai::clientSpeak, this, QString(), sending)));
        }
    }

    void guiOperation()
    {
        auto pair = operateGuiList.takeFirst();
        pair.second();
        operateGui();
    }

    void operateGui()
    {
        if (!operateGuiList.isEmpty()) {
            auto pair = operateGuiList.first();
            QTimer::singleShot(pair.first, this, SLOT(guiOperation()));
        } else
            QTimer::singleShot(100, this, SLOT(operateGui()));
    }
};

Dialog::Dialog(QWidget *parent)
    : QWidget(parent)
{
    listWidget = new QListWidget;
    listWidget->setSortingEnabled(false);

    comboBox = new QComboBox;
    comboBox->setEditable(false);
    comboBox->addItem("all");

    edit = new QLineEdit;
    edit->clear();

    sendbtn = new QPushButton("send");
    connect(sendbtn, &QPushButton::clicked, this, &Dialog::send);

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

void Dialog::playerSpoken(QString from, QString to, QString content, bool fromYou, bool toYou, bool groupsent)
{
    if (fromYou)
        from = tr("you");

    if (toYou)
        to = tr("you");
    else if (groupsent)
        to = tr("__ALLPLAYERS__");

    QString x = tr("%1 said to %2: ").arg(from).arg(to);
    x.append(content);
    listWidget->addItem(x);
    listWidget->scrollToBottom();
}

void Dialog::send()
{
    QString to = comboBox->currentText();
    if (comboBox->currentText() == "all") {
        to.clear();
    }

    client->speak(to, edit->text());
    edit->clear();
}

void Dialog::closeEvent(QCloseEvent *event)
{
    if (client != nullptr)
        client->disconnectFromHost();

    QWidget::closeEvent(event);
}

#include "dialog.moc"
