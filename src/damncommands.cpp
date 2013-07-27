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
	end();

	return true;
}

bool DAmn::login()
{
	begin();
	writeLine(QString("login %1").arg(m_login));
	writeLine(QString("pk=%1").arg(m_token));
	end();

	return true;
}

bool DAmn::join(const QString &channel)
{
	if (channel.isEmpty()) return false;

	begin();
	writeLine(QString("join chat:%1").arg(channel));
	end();

	return true;
}

bool DAmn::joinPrivate(const QStringList &users)
{
	if (users.isEmpty()) return false;

	begin();
	writeLine("join pchat:" + users.join(":"));
	end();

	return true;
}

bool DAmn::part(const QString &channel)
{
	begin();
	writeLine(QString("part chat:%1").arg(channel));
	end();

	return true;
}

bool DAmn::partPrivate(const QString &channel, const QStringList &users)
{
	begin();
	writeLine("part pchat:" + users.join(":"));
	end();

	return true;
}

bool DAmn::pong()
{
	begin();
	writeLine("pong");
	end();

	return true;
}

bool DAmn::sendMessage(const QString &channel, const QString &text)
{
	sendChat(channel);

	writeLine("msg main");
	writeLine();
	writeLine(text);
	end();

	return true;
}

bool DAmn::sendAction(const QString &channel, const QString &text)
{
	sendChat(channel);

	writeLine("action main");
	writeLine();
	writeLine(text);
	end();

	return true;
}

bool DAmn::sendNonParsedMessage(const QString &channel, const QString &text)
{
	sendChat(channel);

	writeLine("npmsg main");
	writeLine();
	writeLine(text);
	end();

	return true;
}

bool DAmn::promote(const QString &channel, const QString &username, const QString &privclass)
{
	sendChat(channel);

	writeLine("promote " + username);

	if (!privclass.isEmpty())
	{
		writeLine();
		writeLine(privclass);
	}

	end();

	return true;
}

bool DAmn::demote(const QString &channel, const QString &username, const QString &privclass)
{
	sendChat(channel);

	writeLine("demote " + username);

	if (!privclass.isEmpty())
	{
		writeLine();
		writeLine(privclass);
	}

	end();

	return true;
}

bool DAmn::ban(const QString &channel, const QString &username)
{
	sendChat(channel);

	writeLine("ban " + username);
	end();

	return true;
}

bool DAmn::unban(const QString &channel, const QString &username)
{
	sendChat(channel);

	writeLine("unban " + username);
	end();

	return true;
}

bool DAmn::kick(const QString &channel, const QString &username, const QString &reason)
{
	begin();
	writeLine("kick chat:" + channel);
	writeLine("u=" + username);

	if (!reason.isEmpty())
	{
		writeLine();
		writeLine(reason);
	}

	end();

	return true;
}

bool DAmn::getChatProperty(const QString &channel, const QString &prop)
{
	// title topic privclasses members
	begin();
	writeLine("get chat:" + channel);
	writeLine("p=" + prop);
	end();

	return true;
}

bool DAmn::getUserInfo(const QString &username)
{
	begin();
	writeLine("get login:" + username);
	writeLine("p=info");
	end();

	return true;
}

bool DAmn::setChatProperty(const QString &channel, const QString &prop, const QString &value)
{
	// title topic
	begin();
	writeLine("set chat:" + channel);
	writeLine("p=" + prop);
	writeLine();
	writeLine(value);
	end();

	return true;
}

bool DAmn::admin(const QString &channel, const QString &command)
{
	sendChat(channel);

	writeLine("admin");
	writeLine();
	writeLine(command);
	end();

	return true;
}

bool DAmn::disconnect()
{
	begin();
	writeLine("disconnect");
	end();

	return true;
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

	end();

	return true;
}

bool DAmn::sendChat(const QString &channel)
{
	begin();
	writeLine("send chat:" + channel);
	writeLine();

	return true;
}

bool DAmn::send(const QString &channel, const QString &text)
{
	if (!text.isEmpty() && text[0] == '/')
	{
		QRegExp reg("([a-zA-Z]+)( (.*))?");

		if (reg.indexIn(text, 1) == 1)
		{
			QString cmd(reg.cap(1));
			QString msg(reg.cap(3));

			if (cmd == "me") return sendAction(channel, msg);
			if (cmd == "part") return part(channel);
			if (cmd == "whois") return getUserInfo(msg);
			if (cmd == "topic") return setChatProperty(channel, "topic", msg);
			if (cmd == "title") return setChatProperty(channel, "title", msg);
			if (cmd == "join") return join(msg);
			if (cmd == "part") return part(msg.isEmpty() ? channel:msg);
			if (cmd == "demote") return demote(channel, msg); // TODO: parse 3rd parameter
			if (cmd == "promote") return promote(channel, msg); // TODO: parse 3rd parameter
			if (cmd == "kick") return kick(channel, msg); // TODO: parse 3rd parameter
			if (cmd == "ban") return ban(channel, msg);
			if (cmd == "unban") return unban(channel, msg);
			if (cmd == "admin") return admin(channel, msg);
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

	return sendMessage(channel, text);
}

bool DAmn::send(const QString &channel, const QStringList &lines)
{
	foreach(const QString &line, lines)
	{
		send(channel, line);
	}

	return true;
}

bool DAmn::begin()
{
	m_writebuffer.clear();

	return true;
}

bool DAmn::writeLine(const QString &line)
{
//	m_writebuffer.append(line.toUtf8() + "\n");
	m_writebuffer.append(line.toLatin1() + "\n");

	return true;
}

bool DAmn::end()
{
	if (!m_socket) return false;

	m_writebuffer.append('\0'); // every message ends with a \0

	return m_socket->write(m_writebuffer) == m_writebuffer.size();
}
