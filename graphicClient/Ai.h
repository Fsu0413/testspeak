#ifndef AI_H
#define AI_H

#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QString>

class QTimer;
class QNetworkAccessManager;
class Dialog;
class Client;

extern "C" {
typedef struct lua_State lua_State;
}

class Ai : public QObject
{
    Q_OBJECT

public:
    Ai(Client *client, Dialog *parent);
    ~Ai();

private:
    void pcall(int argnum);

    Dialog *dialog;
    Client *client;

    QMap<int, QTimer *> timers;

    QNetworkAccessManager *nam1;
    // QList<std::pair<int, std::function<void()>>> operateGuiList;

    lua_State *l;

public:
    // funcs which should be called by lua
    QString name();
    QString gender();
    void queryPlayer(const QString &name);
    void queryTl(const QString &id, const QString &content, const QString &key = QString());
    void addTimer(int timerId, int timeOut);
    void killTimer(int timerId);
    bool setNameCombo(const QString &name);
    void setText(const QString &text);
    void sendPress();
    void sendRelease();
    void sendClick();
    QString getFirstChar(const QString &c);
    QString removeFirstChar(const QString &c);
    void debugOutput(const QString &c);
    void prepareExit();

public slots:
    void addPlayer(QString name);
    void removePlayer(QString name);
    void playerDetail(QJsonObject ob);
    void playerSpoken(QString from, QString to, QString content, bool fromYou, bool toYou, bool groupsent, quint32 time);
    void receive();
    void timeout();
};

#endif
