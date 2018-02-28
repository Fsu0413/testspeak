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

    QMap<QString, QStringList *> speakMap;

public slots:
    void addPlayer(QString name);
    void removePlayer(QString name);
    void playerDetail(QJsonObject ob);
    void playerSpoken(QString from, QString to, QString content, bool fromYou, bool toYou, bool groupsent, quint32 time);

    void send();

    void userNameChanged(QListWidgetItem *current, QListWidgetItem *previous);

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void updateList();
};

#endif // DIALOG_H
