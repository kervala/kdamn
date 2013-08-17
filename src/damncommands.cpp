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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

bool DAmn::client()
{
	begin();
	writeLine("dAmnClient 0.2");
	writeLine(QString("agent=%1 %2").arg(PRODUCT).arg(VERSION));
	return end();
}

bool DAmn::login()
{
	begin();
	writeLine(QString("login %1").arg(m_login));
	writeLine(QString("pk=%1").arg(m_token));
	return end();
}

bool DAmn::join(const QString &room)
{
	if (room.isEmpty()) return false;

	begin();
	writeLine(QString("join chat:%1").arg(room));
	return end();
}

bool DAmn::joinPrivate(const QStringList &users)
{
	if (users.isEmpty()) return false;

	begin();
	writeLine("join pchat:" + users.join(":"));
	return end();
}

bool DAmn::part(const QString &room)
{
	begin();
	writeLine(QString("part chat:%1").arg(room));
	return end();
}

bool DAmn::partPrivate(const QStringList &users)
{
	begin();
	writeLine("part pchat:" + users.join(":"));
	return end();
}

bool DAmn::pong()
{
	begin();
	writeLine("pong");
	return end();
}

bool DAmn::sendMessage(const QString &room, const QString &text)
{
	sendChat(room);

	writeLine("msg main");
	writeLine();
	writeLine(text);
	return end();
}

bool DAmn::sendAction(const QString &room, const QString &text)
{
	sendChat(room);

	writeLine("action main");
	writeLine();
	writeLine(text);
	return end();
}

bool DAmn::sendNonParsedMessage(const QString &room, const QString &text)
{
	sendChat(room);

	writeLine("npmsg main");
	writeLine();
	writeLine(text);
	return end();
}

bool DAmn::promote(const QString &room, const QString &username, const QString &privclass)
{
	sendChat(room);

	writeLine("promote " + username);

	if (!privclass.isEmpty())
	{
		writeLine();
		writeLine(privclass);
	}

	return end();
}

bool DAmn::demote(const QString &room, const QString &username, const QString &privclass)
{
	sendChat(room);

	writeLine("demote " + username);

	if (!privclass.isEmpty())
	{
		writeLine();
		writeLine(privclass);
	}

	return end();
}

bool DAmn::ban(const QString &room, const QString &username)
{
	sendChat(room);

	writeLine("ban " + username);
	return end();
}

bool DAmn::unban(const QString &room, const QString &username)
{
	sendChat(room);

	writeLine("unban " + username);
	return end();
}

bool DAmn::kick(const QString &room, const QString &username, const QString &reason)
{
	begin();
	writeLine("kick chat:" + room);
	writeLine("u=" + username);

	if (!reason.isEmpty())
	{
		writeLine();
		writeLine(reason);
	}

	return end();
}

bool DAmn::getChatProperty(const QString &room, const QString &prop)
{
	// title topic privclasses members
	begin();
	writeLine("get chat:" + room);
	writeLine("p=" + prop);
	return end();
}

bool DAmn::getUserInfo(const QString &username)
{
	begin();
	writeLine("get login:" + username);
	writeLine("p=info");
	return end();
}

bool DAmn::setChatProperty(const QString &room, const QString &prop, const QString &value)
{
	// title topic
	begin();
	writeLine("set chat:" + room);
	writeLine("p=" + prop);
	writeLine();
	writeLine(value);
	return end();
}

bool DAmn::admin(const QString &room, const QString &command)
{
	sendChat(room);

	writeLine("admin");
	writeLine();
	writeLine(command);
	return end();
}

bool DAmn::disconnect()
{
	begin();
	writeLine("disconnect");
	return end();
}

bool DAmn::kill(const QString &username, const QString &reason)
{
	begin();
	writeLine("kill login:" + username);

	if (!reason.isEmpty())
	{
		writeLine();
		writeLine(reason);
	}

	return end();
}

void DAmn::sendChat(const QString &room)
{
	begin();
	writeLine("send chat:" + room);
	writeLine();
}

bool DAmn::send(const QString &room, const QString &text)
{
	if (!text.isEmpty() && text[0] == '/')
	{
		QRegExp reg("([a-zA-Z]+)( (.*))?");

		if (reg.indexIn(text, 1) == 1)
		{
			QString cmd(reg.cap(1));
			QString msg(reg.cap(3));

			if (cmd == "me") return sendAction(room, msg);
			if (cmd == "part") return part(room);
			if (cmd == "whois") return getUserInfo(msg);
			if (cmd == "topic") return setChatProperty(room, "topic", msg);
			if (cmd == "title") return setChatProperty(room, "title", msg);
			if (cmd == "join") return join(msg);
			if (cmd == "part") return part(msg.isEmpty() ? room:msg);
			if (cmd == "demote") return demote(room, msg); // TODO: parse 3rd parameter
			if (cmd == "promote") return promote(room, msg); // TODO: parse 3rd parameter
			if (cmd == "kick") return kick(room, msg); // TODO: parse 3rd parameter
			if (cmd == "ban") return ban(room, msg);
			if (cmd == "unban") return unban(room, msg);
			if (cmd == "admin") return admin(room, msg);
			if (cmd == "clear") return true;
			if (cmd == "stats")
			{
				QMap<QString, int>::const_iterator it = m_stats.begin();

				while(it != m_stats.end())
				{
					emit textReceived("", "", MessageText, QString("%1=%2").arg(it.key()).arg(it.value()), true);

					++it;
				}

				return true;
			}

			if (cmd == "raw")
			{
				// use \\n because \n is escaped
				QStringList lines = msg.split("\\n");

				begin();
				foreach(const QString &line, lines) writeLine(line);
				end();

				return true;
			}

			// help note
			// /admin show users [GroupName]
			// /admin show privclass
		}
	}

	return sendMessage(room, text);
}

bool DAmn::send(const QString &room, const QStringList &lines)
{
	foreach(const QString &line, lines)
	{
		send(room, line);
	}

	return true;
}

void DAmn::begin()
{
	m_writeMutex.lock();

	m_writebuffer.clear();
}

void DAmn::writeLine(const QString &line)
{
	m_writebuffer.append(line.toLatin1() + "\n");
}

bool DAmn::end()
{
	if (!m_socket)
	{
		m_writeMutex.unlock();
		return false;
	}

	m_writebuffer.append('\0'); // every message ends with a \0

	bool res = m_socket->write(m_writebuffer) == m_writebuffer.size();

	m_writeMutex.unlock();

	return res;
}
