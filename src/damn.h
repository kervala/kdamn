/*
 *  kdAmn is a deviantART Messaging Network client
 *  Copyright (C) 2013-2014  Cedric OCHS
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

enum EMessageType
{
	MessageText,
	MessageAction,
	MessageTopic,
	MessageTitle,
	MessageUnknown
};

enum EDAmnError
{
	OK,
	BAD_NAMESPACE,
	BAD_PARAMETER,
	UNKNOWN_PROPERTY,
	AUTHENTICATION_FAILED,
	NOTHING_TO_SEND,
	ALREADY_JOINED,
	NO_LOGIN,
	UNKNOWN,
	LAST_ERROR
};

struct DAmnImage
{
	QString md5;
	QString remoteUrl;
	QString localUrl;
	QString filename;
	bool downloaded;
	bool valid;
	bool oembed;

	DAmnImage():downloaded(false), valid(false), oembed(false)
	{
	}

	bool operator == (const DAmnImage &other)
	{
		return other.md5 == md5;
	}
};

typedef QList<DAmnImage> DAmnImages;
typedef DAmnImages::iterator DAmnImagesIterator;

struct WaitingMessage
{
	QString room;
	QString from;
	QString html;
	DAmnImages images;
	EMessageType type;
};

struct Tablump
{
	QString id;
	int count;
	QString tag;
};

class DAmnRoom;
class DAmnUser;

struct DAmnConnection;

class DAmn : public QObject
{
	Q_OBJECT

public:
	DAmn(QObject *parent);
	virtual ~DAmn();

	void begin();
	void writeLine(const QString &line = "");
	bool end();

	void setLogin(const QString &login);
	void setToken(const QString &token);

	bool connectToServer();

	static DAmn* getInstance() { return s_instance; }

	// dAmn commands
	bool client();
	bool login();
	bool join(const QString &room);
	bool part(const QString &room);
	bool pong();
	bool sendMessage(const QString &room, const QString &text);
	bool sendAction(const QString &room, const QString &text);
	bool sendNonParsedMessage(const QString &room, const QString &text);
	bool promote(const QString &room, const QString &username, const QString &privclass = "");
	bool demote(const QString &room, const QString &username, const QString &privclass = "");
	bool ban(const QString &room, const QString &username);
	bool unban(const QString &room, const QString &username);
	bool kick(const QString &room, const QString &username, const QString &reason = "");
	bool getChatProperty(const QString &room, const QString &prop);
	bool getUserInfo(const QString &username);
	bool setChatProperty(const QString &room, const QString &prop, const QString &value);
	bool admin(const QString &room, const QString &command);
	bool disconnect();
	bool kill(const QString &username, const QString &reason);

	// implemented
	bool send(const QString &room, const QString &text);
	bool send(const QString &room, const QStringList &lines);

	bool downloadImage(DAmnImage &image);
	bool getWaitingMessageFromRemoteUrl(const QString &url, WaitingMessage* &message);

public slots:
	void onConnected();
	void onDisconnected();
	void onStageChanged(QAbstractSocket::SocketState state);
	void onAboutToClose();
	void onReadChannelFinished();

	void onRead();
	void onWritten(qint64 bytes);

	void onError(QAbstractSocket::SocketError error);

	bool onUpdateWaitingMessages(const QString &md5);

signals:
	void authenticationFailedWrongLogin();
	void authenticationFailedWrongToken();
	void serverConnected();

	void textReceived(const QString &room, const QString &user, EMessageType type, const QString &text, bool html);

	void roomJoined(const QString &room);
	void roomParted(const QString &room, const QString &reason);
	void userJoined(const QString &room, const QString &user, bool show);
	void userParted(const QString &room, const QString &user, const QString &reason, bool show);
	void userPrivChanged(const QString &room, const QString &user, const QString &by, const QString &pc);
	void userKicked(const QString &room, const QString &user, const QString &by);
	void usersReceived(const QString &room, const QStringList &users);
	void errorReceived(const QString &error);

private:
	void sendChat(const QString &room);
	bool replaceTablumps(const QString &data, QString &html, QString &text, DAmnImages &images);

	DAmnRoom* createRoom(const QString &room);
	bool removeRoom(const QString &room);
	DAmnRoom* getRoom(const QString &room);
	QString getAvatarUrl(const QString &user, int usericon);

	DAmnUser* createUser(const QString &user);
	DAmnUser* getUser(const QString &user);

	QString getRoomType(const QString &room) const;

	// parser helpers
	bool parseAllMessages(const QStringList &lines);
	bool parsePacket(const QString &cmd, const QStringList &lines, int &i, DAmnPacket &packet);
	void parseProperties(const QStringList &lines, int &i, DAmnProperties &props);
	bool parseUserInfo(const QStringList &lines, int &i, DAmnUser &user);
	bool parseRoomProperty(const QString &room, const DAmnProperties &props, const QStringList &lines, int &i);
	bool parseRoomMembers(const QString &room, const QStringList &lines, int &i);
	bool parseRoomPrivClasses(const QString &room, const QStringList &lines, int &i);
	bool parseUserProperties(const QString &user, const QStringList &lines, int &i);
	bool parseConn(const QStringList &lines, int &i, DAmnConnection &conn);
	bool parseText(const QString &room, const QString &from, EMessageType type, const QStringList &lines, int &i, QString &html, QString &text);
	bool parseJoin(const QString &room, const QString &user, bool show, const QStringList &lines, int &i);
	bool parsePart(const QString &room, const QString &user, bool show, const QString &reason, const QStringList &lines, int &i);
	bool parsePriv(const QString &room, const QString &user, bool show, const QString &by, const QString &pc, const QStringList &lines, int &i);
	bool parseKicked(const QString &room, const QString &user, bool show, const QString &by, const QStringList &lines, int &i);
	bool parseError(const QString &error);

	// parse message sent from server
	bool parseServer(const QStringList &lines);
	bool parseLogin(const QStringList &lines);
	bool parsePing(const QStringList &lines);
	bool parseRecv(const QStringList &lines);
	bool parseSend(const QStringList &lines);
	bool parseJoin(const QStringList &lines);
	bool parsePart(const QStringList &lines);
	bool parseProperty(const QStringList &lines);
	bool parseGet(const QStringList &lines);
	bool parseSet(const QStringList &lines);
	bool parseDisconnect(const QStringList &lines);

	QTcpSocket *m_socket;
	QString m_login;
	QString m_token;
	QString m_serverversion;

	QByteArray m_writeBuffer;
	QByteArray m_readBuffer;

	QList<DAmnUser*> m_users;
	QList<DAmnRoom*> m_rooms;

	QList<WaitingMessage*> m_waitingMessages;

	QMap<QString, int> m_stats;

	QMutex m_writeMutex;
	QDateTime m_lastPing;
	QDateTime m_lastMessage;

	EDAmnError m_lastError;

	static DAmn *s_instance;
};

#endif
