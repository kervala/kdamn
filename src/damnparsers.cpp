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

#include "common.h"
#include "damn.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

bool DAmn::parseAllMessages(const QStringList &lines)
{
	if (parseServer(lines)) return true;
	if (parseLogin(lines)) return true;
	if (parsePing(lines)) return true;
	if (parseRecv(lines)) return true;
	if (parseJoin(lines)) return true;
	if (parseProperty(lines)) return true;
	if (parseGet(lines)) return true;

	qDebug() << lines;

	return false;
}

bool DAmn::parsePacket(const QString &cmd, const QStringList &lines, int &i, DAmnPacket &packet)
{
	// not enough lines
	if (i >= lines.size()) return false;

	// check if the command is right
	if (lines[i].left(cmd.length()) != cmd) return false;

	if (lines[i].length() > cmd.length())
	{
		int pos = cmd.length() + 1;

		QRegExp reg;

		// extract main parameter
		reg.setPattern("([A-Za-z0-9_.~ -]*)");

		if (reg.indexIn(lines[i], pos) == -1) return false;

		packet.cmd = cmd;
		packet.params << reg.cap(1);

		pos += reg.matchedLength();

		reg.setPattern(":([A-Za-z0-9_.~ -]+)");

		while ((pos = reg.indexIn(lines[i], pos)) != -1)
		{
			packet.params << reg.cap(1);
			pos += reg.matchedLength();
		}
	}

	++i;

	// extract all arguments
	if (!parseProperties(lines, i, packet.args)) return false;

	return true;
}

bool DAmn::parseProperties(const QStringList &lines, int &i, DAmnProperties &props)
{
	QRegExp reg("^([a-z]+)=([A-Za-z0-9_.~ -]*)$");

	for(; i < lines.size(); ++i)
	{
		if (lines[i].isEmpty()) break;

		if (!reg.indexIn(lines[i]))
		{
			props[reg.cap(1)] = reg.cap(2);
		}
		else
		{
			qDebug() << lines[i];
		}
	}

	++i;

	return true;
}

bool DAmn::parseUserInfo(const QStringList &lines, int &i, DAmnUser &user)
{
	DAmnProperties props;

	// extract all properties
	if (!parseProperties(lines, i, props)) return false;

	user.usericon = props["usericon"].toInt();
	user.symbol = props["symbol"][0];
	user.realname = props["realname"];
	user.gpc = props["gpc"];

	return true;
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

	if (p.args["e"] != "ok") return false;

	// to be sure it's not an alias
	DAmnUser *user = createUser(p.params[0]);

	if (!parseUserInfo(lines, i, *user)) return false;

	m_step = eStepConnected;

	emit serverConnected();

	return true;
}

bool DAmn::parsePing(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("ping", lines, i, p)) return false;

	return pong();
}

bool DAmn::parseText(const QString &channel, const QString &from, bool action, const QStringList &lines, int &i)
{
	QString text, html;
	QStringList images;

	if (replaceTablumps(lines[i], html, text, images) && m_waitingMessages.isEmpty())
	{
		if (action)
		{
			emit htmlActionReceived(channel, from, html);
			emit textActionReceived(channel, from, text);
		}
		else
		{
			emit htmlMessageReceived(channel, from, html);
			emit textMessageReceived(channel, from, text);
		}
	}
	else
	{
		WaitingMessage *message = new WaitingMessage();
		message->channel = channel;
		message->from = from;
		message->html = html;
		message->images = images;
		message->action = action;

		m_waitingMessages << message;
	}

	return true;
}

bool DAmn::parseJoin(const QString &user, bool show, const QStringList &lines, int &i)
{
	if (show) emit userJoined(user);

	// to be sure it's not an alias
	DAmnUser *u = new DAmnUser();
	u->name = user;

	if (!parseUserInfo(lines, i, *u))
	{
		delete u;

		return false;
	}

	m_users << u;

	return true;
}

bool DAmn::parsePart(const QString &user, bool show, const QString &reason, const QStringList &lines, int &i)
{
	if (show) emit userParted(user, reason);

	return true;
}

bool DAmn::parseRecv(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("recv", lines, i, p)) return false;

	// text
	if (parsePacket("msg", lines, i, p)) return parseText(p.params[1], p.args["from"], false, lines, i);
	if (parsePacket("action", lines, i, p))	return parseText(p.params[1], p.args["from"], true, lines, i);

	// join/part messages
	if (parsePacket("join", lines, i, p)) return parseJoin(p.params[0], p.args["s"] == "1", lines, i);
	if (parsePacket("part", lines, i, p)) return parsePart(p.params[0], p.args["s"] == "1", p.args["r"], lines, i);

	return false;
}

bool DAmn::parseJoin(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("join", lines, i, p)) return false;

	if (p.args["e"] != "ok") return false;

	createChannel(p.params[1]);

	emit channelJoined(p.params[1]);

	return true;
}

bool DAmn::parseChannelProperty(const QString &channel, const DAmnProperties &props, const QStringList &lines, int &i)
{
	DAmnChannel *chan = getChannel(channel);

	if (!chan) return false;

	DAmnChannelProperty prop;
	prop.name = props["p"];
	prop.by = props["by"];
	prop.ts = props["ts"];
	prop.value = lines[i];

	if (prop.name == "topic")
	{
		chan->topic = prop;

		emit topicReceived(channel, prop.value);
	}
	else if (prop.name == "title")
	{
		chan->title = prop;

		emit titleReceived(channel, prop.value);
	}
	else
	{
		qDebug() << "bad property" << prop.name;
	}

	++i;

	return true;
}

bool DAmn::parseChannelMembers(const QString &channel, const QStringList &lines, int &i)
{
	DAmnChannel *chan = getChannel(channel);

	// channel doesn't exist
	if (!chan) return false;

	// update the list of all members, so clear it first
	chan->users.clear();

	while(i < lines.size())
	{
		if (lines[i].isEmpty()) break;

		DAmnPacket p;
		if (!parsePacket("member", lines, i, p)) return false;

		// create user if needed
		DAmnUser *user = createUser(p.params[0]);

		user->usericon = p.args["usericon"].toInt();
		user->symbol = p.args["symbol"][0];
		user->realname = p.args["realname"];
		user->gpc = p.args["gpc"];

		// add member to channel
		DAmnMember member;
		member.name = p.params[0];
		member.pc = p.args["pc"];

		if (chan->users.indexOf(member) < 0) chan->users << member;
	}

	emit membersReceived(channel, chan->users);

	return true;
}

bool DAmn::parseChannelPrivClasses(const QString &channel, const QStringList &lines, int &i)
{
	DAmnChannel *chan = getChannel(channel);

	if (!chan) return false;

	chan->privclasses.clear();

	QRegExp reg("([0-9]+):([A-Z-a-z]+)");

	for(; i < lines.size(); ++i)
	{
		if (lines[i].isEmpty()) break;

		if (reg.indexIn(lines[i]) == 0)
		{
			DAmnPrivClass privclass;
			privclass.id = reg.cap(1).toInt();
			privclass.name = reg.cap(2);

			chan->privclasses << privclass;
		}
		else
		{
			qDebug() << "error";
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

	conn.channel = reg.cap(1);

	++i;
	++i;

	return true;
}

bool DAmn::parseUserProperties(const QString &user, const QStringList &lines, int &i)
{
	bool found = false;

	DAmnUser *u = createUser(user);

	if (!parseUserInfo(lines, i, *u)) return false;

	while(i < lines.size())
	{
		DAmnConnection c;

		if (!parseConn(lines, i, c)) break;

		u->connections[c.channel] = c;
	}

	return true;
}

bool DAmn::parseProperty(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("property", lines, i, p)) return false;

	if (p.params[0] == "chat")
	{
		// extract all arguments
		if (p.args["p"] == "members") return parseChannelMembers(p.params[1], lines, i);
		if (p.args["p"] == "privclasses") return parseChannelPrivClasses(p.params[1], lines, i);
		if (p.args["p"] == "topic" || p.args["p"] == "title") return parseChannelProperty(p.params[1], p.args, lines, i);
	}
	else if (p.params[0] == "login")
	{
		if (p.args["p"] == "info") return parseUserProperties(p.params[1], lines, i);
	}

	qDebug() << "arg" << p.args["p"];

	return false;
}

bool DAmn::parseGet(const QStringList &lines)
{
	int i = 0;

	DAmnPacket p;

	if (!parsePacket("get", lines, i, p)) return false;

//	if (p.param1 == "login" && p.args["p"] == "info") return parseUserProperties(p.param2, lines, i);

	if (p.args["e"] != "ok") return false;

	return true;
}
