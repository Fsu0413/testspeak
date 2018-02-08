#ifndef DIALOG_H
#define DIALOG_H

#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QPushButton>
#include <QString>
#include <QTcpSocket>
#include <QWidget>

class Client;

class Dialog : public QWidget
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = 0);
    ~Dialog();

    QMap<QString, QString> lastSent;
    QMap<QString, QString> lastRecv;

    QListWidget *listWidget;
    QComboBox *comboBox;
    QLineEdit *edit;
    QPushButton *sendbtn;

    Client *client;

public slots:
    void addPlayer(QString name);
    void removePlayer(QString name);
    void playerDetail(QJsonObject ob);
    void playerSpoken(QString from, QString to, QString content, bool fromYou, bool toYou, bool groupsent);

    void send();

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // DIALOG_H
