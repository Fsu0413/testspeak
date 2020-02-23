#ifndef DIALOG_H
#define DIALOG_H

#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QString>

#ifdef GRAPHICSCLIENT
#include <QLineEdit>
#include <QWidget>
#endif

class Client;
class QThread;

#ifdef GRAPHICSCLIENT
class QCloseEvent;
class QListWidget;
class QListWidgetItem;
class QPushButton;
#endif

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

#ifdef GRAPHICSCLIENT
class AutomatedLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    void mousePressEvent(QMouseEvent *event) override
    {
        QLineEdit::mousePressEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        QLineEdit::mouseReleaseEvent(event);
    }
};
#else

struct PlayerDetail
{
    QString name;
    QString gender;
    bool hasUnreadMessage;
};

#endif

class Dialog
#ifdef GRAPHICSCLIENT
    : public QWidget
#else
    : public QObject
#endif
{
    Q_OBJECT

public:
#ifdef GRAPHICSCLIENT
    Dialog(QWidget *parent = nullptr);
#else
    Dialog(QObject *parent = nullptr);
#endif
    ~Dialog();

    Client *client;
    QThread *aiThread;

    QMap<QString, QList<SpeakDetail> *> speakMap;

#ifdef GRAPHICSCLIENT
    QListWidget *listWidget;
    QListWidget *userNames;
    AutomatedLineEdit *edit;
    QPushButton *sendbtn;

#else
    QMap<QString, PlayerDetail *> playerMap;

    QString speakTo;
    QString edit;
#endif

public slots:
    void addPlayer(QString name, QString gender);
    void removePlayer(QString name);
    void playerDetail(QJsonObject ob);
    void playerSpoken(QString from, QString to, QString content, bool fromYou, bool toYou, bool groupsent, quint32 time);

    void send();

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

    void userNameChanged(
#ifdef GRAPHICSCLIENT
        QListWidgetItem *current, QListWidgetItem *previous
#endif
    );

#ifdef GRAPHICSCLIENT
    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
#endif

private:
    void updateList();
};

#endif // DIALOG_H
