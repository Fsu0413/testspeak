#ifndef DIALOG_H
#define DIALOG_H

#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QWidget>

class QCloseEvent;
class QListWidget;
class QLineEdit;
class QPushButton;
class Client;

class Dialog : public QWidget
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

    QMap<QString, QString> lastSent;
    QMap<QString, QString> lastRecv;

    QListWidget *listWidget;
    QListWidget *comboBox;
    QLineEdit *edit;
    QPushButton *sendbtn;

    Client *client;

public slots:
    void addPlayer(QString name);
    void removePlayer(QString name);
    void playerDetail(QJsonObject ob);
    void playerSpoken(QString from, QString to, QString content, bool fromYou, bool toYou, bool groupsent, quint32 time);

    void send();

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // DIALOG_H
