/*
 *  kdAmn is a deviantART Messaging Network client
 *  Copyright (C) 2013  Cedric OCHS
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef DAMN_H
#define DAMN_H

typedef QMap<QString, QString> DAmnProperties;

struct DAmnPacket
{
	QString cmd;
	QStringList params;
	DAmnProperties args;
};

struct DAmnConnection
{
	QString channel;
	int online;
	int idle;
};

struct DAmnMember
{
	QString name;
	QString pc;

	bool operator == (const DAmnMember &other) { return other.name == name; }
};

struct DAmnPrivClass
{
	int id;
	QString name;
};

struct DAmnUser
{
	QString name;
	int usericon;
	QChar symbol;
	QString realname;
	QString gpc;

	QMap<QString, DAmnConnection> connections;
};

struct DAmnChannelProperty
{
	QString name;
	QString value;
	QString by;
	QString ts;
};

struct DAmnChannel
{
	QString name;
	DAmnChannelProperty topic;
	DAmnChannelProperty title;

	QList<DAmnPrivClass> privclasses;
	QList<DAmnMember> users;
};

struct WaitingMessage
{
	QString channel;
	QString from;
	QString html;
	QStringList images;
	bool action;
};

struct Tablump
{
	QString id;
	int count;
	QString tag;
};

class DAmn : public QObject
{
	Q_OBJECT

public:
	DAmn(QObject *parent);
	virtual ~DAmn();

	bool begin();
	bool writeLine(const QString &line = "");
	bool end();

	void setLogin(const QString &login);
	void setToken(const QString &token);

	bool connectToServer();

	static DAmn* getInstance() { return s_instance; }

public slots:
	// dAmn commands
	bool client();
	bool login();
	bool join(const QString &channel);
	bool joinPrivate(const QStringList &users);
	bool part(const QString &channel);
	bool partPrivate(const QString &channel, const QStringList &users);
	bool pong();
	bool sendMessage(const QString &channel, const QString &text);
	bool sendAction(const QString &channel, const QString &text);
	bool sendNonParsedMessage(const QString &channel, const QString &text);
	bool promote(const QString &channel, const QString &username, const QString &privclass = "");
	bool demote(const QString &channel, const QString &username, const QString &privclass = "");
	bool ban(const QString &channel, const QString &username);
	bool unban(const QString &channel, const QString &username);
	bool kick(const QString &channel, const QString &username, const QString &reason = "");
	bool getChatProperty(const QString &channel, const QString &prop);
	bool getUserInfo(const QString &username);
	bool setChatProperty(const QString &channel, const QString &prop, const QString &value);
	bool admin(const QString &channel, const QString &command);
	bool disconnect();
	bool kill(const QString &username, const QString &reason);

	// implemented
	bool send(const QString &channel, const QString &text);
	void onError(QAbstractSocket::SocketError error);
	bool read();
	bool updateWaitingMessages(const QString &md5);

signals:
	void authenticationFailed();
	void serverConnected();
	void topicReceived(const QString &channel, const QString &topic);
	void titleReceived(const QString &channel, const QString &title);
	void htmlMessageReceived(const QString &channel, const QString &user, const QString &html);
	void textMessageReceived(const QString &channel, const QString &user, const QString &text);
	void htmlActionReceived(const QString &channel, const QString &user, const QString &html);
	void textActionReceived(const QString &channel, const QString &user, const QString &text);
	void imageDownloaded(const QString &md5);
	void channelJoined(const QString &channel);
	void channelParted(const QString &channel, const QString &reason);
	void userJoined(const QString &user);
	void userParted(const QString &user, const QString &reason);
	void membersReceived(const QString &channel, const QList<DAmnMember> &members);
	void errorReceived(const QString &error);

private:
	enum eStep
	{
		eStepNone,
		eStepCookies,
		eStepAuthToken,
		eStepConnected
	};

	bool sendChat(const QString &channel);
	bool replaceTablumps(const QString &data, QString &html, QString &text, QStringList &images);

	bool downloadImage(const QString &url, QString &file, QString &md5);

	DAmnChannel* createChannel(const QString &channel);
	bool removeChannel(const QString &channel);
	DAmnChannel* getChannel(const QString &channel);

	DAmnUser* createUser(const QString &user);
	DAmnUser* getUser(const QString &user);

	// parser helpers
	bool parseAllMessages(const QStringList &lines);
	bool parsePacket(const QString &cmd, const QStringList &lines, int &i, DAmnPacket &apcket);
	void parseProperties(const QStringList &lines, int &i, DAmnProperties &props);
	bool parseUserInfo(const QStringList &lines, int &i, DAmnUser &user);
	bool parseChannelProperty(const QString &channel, const DAmnProperties &props, const QStringList &lines, int &i);
	bool parseChannelMembers(const QString &channel, const QStringList &lines, int &i);
	bool parseChannelPrivClasses(const QString &channel, const QStringList &lines, int &i);
	bool parseUserProperties(const QString &user, const QStringList &lines, int &i);
	bool parseConn(const QStringList &lines, int &i, DAmnConnection &conn);
	bool parseText(const QString &channel, const QString &from, bool action, const QStringList &lines, int &i);
	bool parseJoin(const QString &user, bool show, const QStringList &lines, int &i);
	bool parsePart(const QString &user, bool show, const QString &reason, const QStringList &lines, int &i);
	bool parseError(const QString &error);

	// parse message sent from server
	bool parseServer(const QStringList &lines);
	bool parseLogin(const QStringList &lines);
	bool parsePing(const QStringList &lines);
	bool parseRecv(const QStringList &lines);
	bool parseJoin(const QStringList &lines);
	bool parsePart(const QStringList &lines);
	bool parseProperty(const QStringList &lines);
	bool parseGet(const QStringList &lines);
	bool parseSet(const QStringList &lines);
	bool parseDisconnect(const QStringList &lines);

	QTcpSocket *m_socket;
	QString m_login;
	QString m_token;
	QByteArray m_writebuffer;
	char *m_readbuffer;
	qint64 m_buffersize;
	QString m_serverversion;

	QList<DAmnUser*> m_users;
	QList<DAmnChannel*> m_channels;

	QList<WaitingMessage*> m_waitingMessages;

	static DAmn *s_instance;
};

#endif
