#ifndef DIALOG_H
#define DIALOG_H

#include <QLineEdit>
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

    QString lastSent;
    QString lastRecv;

    QListWidget *listWidget;
    QLineEdit *edit;
    QPointer<QTcpSocket> socket;

    void setStuckString();
    void setGreetString();
    void defeatDup();

    void startTyping();
    void addSymbol();

    void getStringFromFile(const QString &);

public slots:
    void sendPack();
    void send1stPack();

    void handleNewConnection();

    void receive();

    void receiveFromClient();
    void sendToClient();
    void typing();
};

#endif // DIALOG_H
