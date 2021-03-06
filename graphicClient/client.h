#ifndef CLIENT_H
#define CLIENT_H

#include <QAbstractSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QString>

class QTcpSocket;

enum ServerProtocol
{
    SP_Zero,

    SP_SignIn,
    SP_Query,
    SP_Speak,
    SP_SignOut,

    SP_Max,
};

enum ClientProtocol
{
    CP_Zero,

    CP_SignedIn,
    CP_SignedOut,
    CP_QueryResult,
    CP_Spoken,

    CP_Max,
};

class Client : public QObject
{
    Q_OBJECT

private:
    void writeJsonDocument(const QJsonDocument &doc);
    void resendMessage();

public:
    Client(QObject *parent = nullptr);

    typedef void (Client::*ClientFunction)(const QJsonObject &content);
    static const Client::ClientFunction ClientFunctions[CP_Max];

    void connectToHost(const QString &host, int port);
    void disconnectFromHost();

    void notifiedHeartBeat(const QJsonObject &contents);
    void notifiedSignedIn(const QJsonObject &contents);
    void notifiedSignedOut(const QJsonObject &contents);
    void notifiedQueryResult(const QJsonObject &contents);
    void notifiedSpoken(const QJsonObject &contents);

public slots:
    void signIn();
    void lostConnection();
    void socketError(QAbstractSocket::SocketError e);
    void socketReadyRead();
    void queryPlayerDetail(QString name);
    void speak(QString to, QString content);

    void sendHeartBeat();

signals:
    void addPlayer(QString name, QString gender);
    void removePlayer(QString name);
    void playerDetail(QJsonObject ob);
    void playerSpoken(QString from, QString to, QString content, bool fromYou, bool toYou, bool groupsent, quint32 time);

    void signedIn();
};

#endif
