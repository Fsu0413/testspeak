#ifndef DIALOG_H
#define DIALOG_H

#include <QListWidget>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QTcpSocket>
#include <QWidget>

class Dialog : public QWidget
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = 0);
    ~Dialog();

    QNetworkAccessManager *nam1;

    QString str;
    int to;

    QListWidget *listWidget;
    QPointer<QTcpSocket> socket;

    void setStuckString();
public slots:
    void sendPack();
    void send1stPack();

    void handleNewConnection();

    void receive();

    void receiveFromClient();
    void sendToClient();
};

#endif // DIALOG_H
