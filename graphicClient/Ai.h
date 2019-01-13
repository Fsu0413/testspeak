#ifndef AI_H
#define AI_H

#ifdef GRAPHICSCLIENT
#include "dialog.h"
#else
#include "console.h"
#endif

#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QString>

class QTimer;
class QNetworkAccessManager;
class Client;
class QMutex;

extern "C" {
typedef struct lua_State lua_State;
}

extern QVariantMap AiData;
extern QMutex AiDataMutex;

class Ai : public QObject
{
    Q_OBJECT

public:
    Ai(Dialog *dialog);
    ~Ai();

private:
    void pcall(int argnum);

    Dialog *dialog;

    QMap<int, QTimer *> timers;

    QNetworkAccessManager *nam1;
    // QList<std::pair<int, std::function<void()>>> operateGuiList;

    lua_State *l;

public:
    // funcs which should be called by lua
    QString name();
    QString gender();
    QString getPlayerGender(const QString &name);
    void queryTl(const QString &id, const QString &content, const QString &key = QString(), const QString &aiComment = QString());
    void addTimer(int timerId, int timeOut);
    void killTimer(int timerId);
    QString getFirstChar(const QString &c);
    QString removeFirstChar(const QString &c);
    void debugOutput(const QString &c);
    QStringList newMessagePlayers();
    QStringList onlinePlayers();
    SpeakDetail getNewestSpokenMessage();

public slots:
    void receive();
    void timeout();

    void start();

signals:
    // queued connection
    void setNameCombo(const QString &name);
    void setText(const QString &text);
    void setTextFocus();
    void sendPress();
    void sendRelease();
    void sendClick();

    // blocking queued connection
    void refreshPlayerList();
    void refreshMessageList();
};

#endif
