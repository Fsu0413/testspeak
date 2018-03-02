#ifndef DIALOG_H
#define DIALOG_H

#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QWidget>

class QCloseEvent;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QPushButton;
class Client;
class QThread;

struct SpeakDetail
{
    QString from;
    bool fromYou;
    bool toYou;
    bool groupSent;
    quint32 time;
    QString content;
};

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

class Dialog : public QWidget
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

    QListWidget *listWidget;
    QListWidget *userNames;
    QLineEdit *edit;
    QPushButton *sendbtn;

    Client *client;
    QThread *aiThread;

    QMap<QString, QList<SpeakDetail> *> speakMap;

public slots:
    void addPlayer(QString name, QString gender);
    void removePlayer(QString name);
    void playerDetail(QJsonObject ob);
    void playerSpoken(QString from, QString to, QString content, bool fromYou, bool toYou, bool groupsent, quint32 time);

    void send();

    void userNameChanged(QListWidgetItem *current, QListWidgetItem *previous);

    // queued connection
    void setNameCombo(const QString &name);
    void setText(const QString &text);
    void sendPress();
    void sendRelease();
    void sendClick();

    // blocking queued connection
    void refreshPlayerList();
    void refreshMessageList();

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void updateList();
};

#endif // DIALOG_H
