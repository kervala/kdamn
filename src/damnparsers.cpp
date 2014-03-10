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

#include "common.h"
#include "damnroom.h"
#include "damnuser.h"
#include "damn.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

struct DAmnError
{
	QString name;
	QString description;
};

enum EDAmnError
{
	BAD_NAMESPACE,
	BAD_PARAMETER,
	UNKNOWN_PROPERTY,
	AUTHENTICATION_FAILED,
	NOTHING_TO_SEND,
	ALREADY_JOINED,
	NO_LOGIN,
	LAST_ERROR
};

bool DAmn::parseAllMessages(const QStringList &lines)
{
	if (parsePing(lines)) return true; // every 5 minutes 18
	if (parseRecv(lines)) return true; // each message 31
	if (parseProperty(lines)) return true; // 5
	if (parseJoin(lines)) return true; // 2
	if (parsePart(lines)) return true; // 2

	if (parseGet(lines)) return true; // 1
	if (parseSet(lines)) return true; // 1
	if (parseSend(lines)) return true; // when error only

	if (parseServer(lines)) return true; // once
	if (parseLogin(lines)) return true; // once
	if (parseDisconnect(lines)) return true; // once

	emit errorReceived(tr("Unable to recognize message: %1").arg(lines[0]));

	return false;
}

bool DAmn::parsePacket(const QString &cmd, const QStringList &lines, int &i, DAmnPacket &packet)
{
	// not enough lines
	if (i >= lines.size()) return false;

	// check if the command is right
	if (!lines[i].startsWith(cmd)) return false;

	if (m_stats.contains(cmd)) ++m_stats[cmd]; else m_stats[cmd] = 0;

	packet.cmd = cmd;
	packet.params.clear();

	if (lines[i].length() > cmd.length())
	{
		int pos = cmd.length() + 1;

		QRegExp reg;

		// extract main parameter
		reg.setPattern("([A-Za-z0-9_.~ -]*)");

		if (reg.indexIn(lines[i], pos) == pos)
		{
			packet.params << reg.cap(1);

			pos += reg.matchedLength();

			reg.setPattern(":([:A-Za-z0-9_.~ -]+)");

			while ((pos = reg.indexIn(lines[i], pos)) != -1)
			{
				packet.params << reg.cap(1);
				pos += reg.matchedLength();
			}
		}
		else
		{
			emit errorReceived(tr("Unable to parse parameter: %1").arg(lines[i]));

			return false;
		}

	}

	++i;

	// extract all arguments
	parseProperties(lines, i, packet.args);

	return true;
}

void DAmn::parseProperties(const QStringList &lines, int &i, DAmnProperties &props)
{
	props.clear();

	QRegExp reg("^([a-z]+)=(.*)$");

	for(; i < lines.size(); ++i)
	{
		if (lines[i].isEmpty()) break;

		if (!reg.indexIn(lines[i]))
		{
			props[reg.cap(1)] = reg.cap(2);
		}
		else
		{
			emit errorReceived(tr("Unable to parse property: %1").arg(lines[i]));
		}
	}

	++i;
}

bool DAmn::parseUserInfo(const QStringList &lines, int &i, DAmnUser &user)
{
	DAmnProperties props;

	// extract all properties
	parseProperties(lines, i, props);

	if (props["symbol"].isEmpty()) return false;

	// assign properties to user
	user.setUserIcon(props["usericon"].toInt());
	user.setSymbol(props["symbol"][0]);
	user.setRealName(props["realname"]);
	user.setGPC(props["gpc"]);

	return true;
}

bool DAmn::parseError(const QString &error)
{
	if (error == "ok") return true;

	static DAmnError s_errors[] =
	{
		{ "bad namespace", tr("Bad namespace") },
		{ "bad parameter", tr("Bad parameter") },
		{ "unknown property", tr("Unknown property") },
		{ "authentication failed", tr("Authentication failed") },
		{ "nothing to send", tr("Nothing to send") },
		{ "already joined", tr("Already joined") },
		{ "no login", tr("No login") },
		{ "", "" }
	};

	for(int i = 0; !s_errors[i].name.isEmpty(); ++i)
	{
		if (error == s_errors[i].name)
		{
			emit errorReceived(s_errors[i].description);

			return false;
		}
	}

	emit errorReceived(tr("Error \"%1\" not recognized").arg(error));

	return false;
}

bool DAmn::parseServer(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("dAmnServer", lines, i, p)) return false;

	m_serverversion = p.params[0];

	return login();
}

bool DAmn::parseLogin(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("login", lines, i, p)) return false;

	if (!parseError(p.args["e"]))
	{
		emit authenticationFailed();

		return true;
	}

	// to be sure it's not an alias
	DAmnUser *user = createUser(p.params[0]);

	if (!parseUserInfo(lines, i, *user)) return true;

	emit serverConnected();

	return true;
}

bool DAmn::parsePing(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("ping", lines, i, p)) return false;

	m_lastPing = QDateTime::currentDateTime();

	return pong();
}

bool DAmn::parseText(const QString &room, const QString &from, MessageType type, const QStringList &lines, int &i, QString &html, QString &text)
{
	DAmnImages images;

	if (replaceTablumps(lines[i], html, text, images) && m_waitingMessages.isEmpty())
	{
		emit textReceived(room, from, type, html, true);
	}
	else
	{
		WaitingMessage *message = new WaitingMessage();
		message->room = room;
		message->from = from;
		message->html = html;
		message->images = images;
		message->type = type;

		m_waitingMessages << message;
	}

	emit textReceived(room, from, type, text, false);

	++i;

	return true;
}

bool DAmn::parseJoin(const QString &room, const QString &user, bool show, const QStringList &lines, int &i)
{
	// to be sure it's not an alias
	DAmnUser *u = new DAmnUser(user, this);

	if (!parseUserInfo(lines, i, *u))
	{
		delete u;

		return false;
	}

	m_users << u;

	DAmnRoom *c = getRoom(room);

	if (!c) return false;

	if (c->addUser(user)) emit userJoined(room, user, show);

	return true;
}

bool DAmn::parsePart(const QString &room, const QString &user, bool show, const QString &reason, const QStringList &lines, int &i)
{
	DAmnRoom *c = getRoom(room);

	if (!c) return false;

	if (c->removeUser(user))
	{
		emit userParted(room, user, reason, show);
	}

	return true;
}

bool DAmn::parsePriv(const QString &room, const QString &user, bool show, const QString &by, const QString &pc, const QStringList &lines, int &i)
{
	if (show) emit userPrivChanged(room, user, by, pc);

	DAmnRoom *c = getRoom(room);

	if (!c) return false;

	DAmnRoomUser member;
	member.name = user;
	member.pc = pc;
	member.by = by;

	c->setUser(member);

	return true;
}

bool DAmn::parseKicked(const QString &room, const QString &user, bool show, const QString &by, const QStringList &lines, int &i)
{
	if (show) emit userKicked(room, user, by);

	DAmnRoom *r = getRoom(room);

	if (!r) return false;

	qDebug() << "Not implemented";

	return true;
}

bool DAmn::parseRecv(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("recv", lines, i, p)) return false;

	if (p.params[0] == "chat")
	{
		QString room = p.params[1];
		QString html, text;

		// text
		if (parsePacket("msg", lines, i, p)) return parseText(room, p.args["from"], MessageText, lines, i, html, text);
		if (parsePacket("action", lines, i, p))	return parseText(room, p.args["from"], MessageAction, lines, i, html, text);

		// join/part messages
		if (parsePacket("join", lines, i, p)) return parseJoin(room, p.params[0], p.args["s"] == "1", lines, i);
		if (parsePacket("part", lines, i, p)) return parsePart(room, p.params[0], p.args["s"] == "1", p.args["r"], lines, i);

		// promote/demote
		if (parsePacket("privchg", lines, i, p)) return parsePriv(room, p.params[0], p.args["i"] == "1", p.args["by"], p.args["pc"], lines, i);

		// kicked
		if (parsePacket("kicked", lines, i, p)) return parseKicked(room, p.params[0], p.args["i"] == "1", p.args["by"], lines, i);
	}
	else if (p.params[0] == "pchat")
	{
		QString room = p.params[1];
		QString html, text;

		// text
		if (parsePacket("msg", lines, i, p)) return parseText(room, p.args["from"], MessageText, lines, i, html, text);
		if (parsePacket("action", lines, i, p))	return parseText(room, p.args["from"], MessageAction, lines, i, html, text);

		// join/part messages
		if (parsePacket("join", lines, i, p)) return parseJoin(room, p.params[0], p.args["s"] == "1", lines, i);
		if (parsePacket("part", lines, i, p)) return parsePart(room, p.params[0], p.args["s"] == "1", p.args["r"], lines, i);

		// promote/demote
		if (parsePacket("privchg", lines, i, p)) return parsePriv(room, p.params[0], p.args["i"] == "1", p.args["by"], p.args["pc"], lines, i);
	}
	else
	{
		emit errorReceived(tr("Unable to recognize param: %1").arg(p.params[0]));
	}

	emit errorReceived(tr("Unable to recognize command: %1").arg(lines[i]));

	return true;
}

bool DAmn::parseSend(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("send", lines, i, p)) return false;

	if (!parseError(p.args["e"])) return true;

	emit errorReceived(tr("Not implemented: %1").arg(lines[i]));

	return true;
}

bool DAmn::parseJoin(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("join", lines, i, p)) return false;

	if (!parseError(p.args["e"])) return true;

	if (p.params[0] == "chat")
	{
		createRoom(p.params[1]);

		emit roomJoined(p.params[1]);
	}
	else if (p.params[0] == "pchat")
	{
		createRoom(p.params[1]);

		emit roomJoined(p.params[1]);
	}
	else
	{
		emit errorReceived(tr("Not implemented: %1").arg(lines[i]));
	}

	return true;
}

bool DAmn::parsePart(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("part", lines, i, p)) return false;

	if (!parseError(p.args["e"])) return true;

	removeRoom(p.params[1]);

	emit roomParted(p.params[1], p.args["p"]);

	return true;
}

bool DAmn::parseRoomProperty(const QString &room, const DAmnProperties &props, const QStringList &lines, int &i)
{
	DAmnRoom *chan = getRoom(room);

	if (!chan) return false;

	DAmnRoomProperty prop;
	prop.name = props["p"];
	prop.by = props["by"];
	prop.ts = props["ts"];

	MessageType type = MessageUnknown;

	if (prop.name == "topic")
	{
		type = MessageTopic;
	}
	else if (prop.name == "title")
	{
		type = MessageTitle;
	}
	else
	{
		emit errorReceived(tr("Unable to recognize property: %1").arg(prop.name));
	}

	parseText(room, prop.by, type, lines, i, prop.html, prop.text);

	chan->setProperty(prop);

	return true;
}

bool DAmn::parseRoomMembers(const QString &room, const QStringList &lines, int &i)
{
	DAmnRoom *chan = getRoom(room);

	// room doesn't exist
	if (!chan) return false;

	QStringList list;

	// update the list of all members, so clear it first
	chan->removeUsers();

	while(i < lines.size())
	{
		if (lines[i].isEmpty()) break;

		DAmnPacket p;
		if (!parsePacket("member", lines, i, p)) return false;

		// create user if needed
		DAmnUser *user = createUser(p.params[0]);

		user->setUserIcon(p.args["usericon"].toInt());
		user->setSymbol(p.args["symbol"][0]);
		user->setRealName(p.args["realname"]);
		user->setGPC(p.args["gpc"]);

		// add member to room
		DAmnRoomUser member;
		member.name = p.params[0];
		member.pc = p.args["pc"];
		member.count = 1;

		if (chan->addUser(member)) list << member.name;
	}

	emit usersReceived(room, list);

	return true;
}

bool DAmn::parseRoomPrivClasses(const QString &room, const QStringList &lines, int &i)
{
	DAmnRoom *chan = getRoom(room);

	if (!chan) return false;

	chan->removePrivClasses();

	QRegExp reg("([0-9]+):([A-Z-a-z]+)");

	for(; i < lines.size(); ++i)
	{
		if (lines[i].isEmpty()) break;

		if (reg.indexIn(lines[i]) == 0)
		{
			DAmnPrivClass privclass;
			privclass.id = reg.cap(1).toInt();
			privclass.name = reg.cap(2);

			chan->setPrivClass(privclass);
		}
		else
		{
			emit errorReceived(tr("Unable to parse privclass: %1").arg(lines[i]));
		}
	}

	return true;
}

bool DAmn::parseConn(const QStringList &lines, int &i, DAmnConnection &conn)
{
	DAmnPacket p;

	if (!parsePacket("conn", lines, i, p)) return false;

	conn.idle = p.args["idle"].toInt();
	conn.online = p.args["online"].toInt();

	QRegExp reg("ns chat:([A-Za-z0-9_.-]+)");

	if (reg.indexIn(lines[i])) return false;

	conn.room = reg.cap(1);

	++i;
	++i;

	return true;
}

bool DAmn::parseUserProperties(const QString &user, const QStringList &lines, int &i)
{
	DAmnUser *u = createUser(user);

	if (!parseUserInfo(lines, i, *u)) return false;

	u->removeConnections();

	while(i < lines.size())
	{
		DAmnConnection c;

		if (!parseConn(lines, i, c)) break;

		u->setConnection(c);
	}

	return true;
}

bool DAmn::parseProperty(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("property", lines, i, p)) return false;

	QString prop = p.args["p"];

	if (p.params[0] == "chat")
	{
		// extract all arguments
		if (prop == "members") return parseRoomMembers(p.params[1], lines, i);
		if (prop == "privclasses") return parseRoomPrivClasses(p.params[1], lines, i);
		if (prop == "topic" || prop == "title") return parseRoomProperty(p.params[1], p.args, lines, i);
	}
	else if (p.params[0] == "login")
	{
		if (prop == "info") return parseUserProperties(p.params[1], lines, i);
	}
	else if (p.params[0] == "pchat")
	{
		// extract all arguments
		if (prop == "members") return parseRoomMembers(p.params[1], lines, i);
		if (prop == "topic" || prop == "title") return parseRoomProperty(p.params[1], p.args, lines, i);
	}
	else
	{
		emit errorReceived(tr("Not implemented: %1").arg(p.params[0]));
	}

	emit errorReceived(tr("Unable to recognize property: %1").arg(prop));

	return false;
}

bool DAmn::parseGet(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("get", lines, i, p)) return false;

	if (!parseError(p.args["e"])) return false;

	emit errorReceived(tr("Not implemented: %1").arg(lines[i]));

	return true;
}

bool DAmn::parseSet(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("set", lines, i, p)) return false;

	if (!parseError(p.args["e"])) return false;

	emit errorReceived(tr("Not implemented: %1").arg(lines[i]));

	return true;
}

bool DAmn::parseDisconnect(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("disconnect", lines, i, p)) return false;

	if (!parseError(p.args["e"])) return true;

	emit errorReceived(tr("Not implemented: %1").arg(lines[i]));

	return true;
}
