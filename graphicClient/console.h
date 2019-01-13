#ifndef CONSOLE_H
#define CONSOLE_H

#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QString>

class Client;
class QThread;

enum SpeakRole
{
    SpeakRole__QtUserRole = Qt::UserRole,

    FromRole,
    FromYouRole,
    ToYouRole,
    GroupSentRole,
    TimeRole,
    ContentRole,
};

enum PlayerRole
{
    PlayerRole__QtUserRole = Qt::UserRole,

    HasUnreadMessageRole,
    GenderRole,
};

struct SpeakDetail
{
    QString from;
    bool fromYou;
    bool toYou;
    bool groupSent;
    quint32 time;
    QString content;
};

struct PlayerDetail
{
    QString name;
    QString gender;
    bool hasUnreadMessage;
};

class ConsoleButton;

class Dialog : public QObject
{
    Q_OBJECT

public:
    Dialog(QObject *parent = nullptr);
    ~Dialog();

    Client *client;
    QThread *aiThread;

    QMap<QString, QList<SpeakDetail> *> speakMap;
    QMap<QString, PlayerDetail *> playerMap;

    QString speakTo;
    QString edit;
    ConsoleButton *sendBtn;

public slots:
    void addPlayer(QString name, QString gender);
    void removePlayer(QString name);
    void playerDetail(QJsonObject ob);
    void playerSpoken(QString from, QString to, QString content, bool fromYou, bool toYou, bool groupsent, quint32 time);

    void send();
    void userNameChanged();

    // queued connection
    void setNameCombo(const QString &name);
    void setTextFocus();
    void setText(const QString &text);
    void sendPress();
    void sendRelease();
    void sendClick();

    // blocking queued connection
    void refreshPlayerList();
    void refreshMessageList();

private:
    void updateList();
};

#endif // DIALOG_H
