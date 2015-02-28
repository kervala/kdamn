/*
 *  kdAmn is a deviantART Messaging Network client
 *  Copyright (C) 2013-2015  Cedric OCHS
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
#include "damnuser.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

DAmnUser::DAmnUser(const QString &name, QObject *parent):QObject(parent), m_name(name)
{
}

DAmnUser::~DAmnUser()
{
}

QString DAmnUser::getName() const
{
	return m_name;
}

void DAmnUser::setName(const QString &name)
{
	m_name = name;
}

int DAmnUser::getUserIcon() const
{
	return m_usericon;
}

void DAmnUser::setUserIcon(int usericon)
{
	m_usericon = usericon;
}

QChar DAmnUser::getSymbol() const
{
	return m_symbol;
}

void DAmnUser::setSymbol(const QChar &symbol)
{
	m_symbol = symbol;
}

QString DAmnUser::getRealName() const
{
	return m_realname;
}

void DAmnUser::setRealName(const QString &realname)
{
	m_realname = realname;
}

QString DAmnUser::getGPC() const
{
	return m_gpc;
}

void DAmnUser::setGPC(const QString &gpc)
{
	m_gpc = gpc;
}

DAmnConnections DAmnUser::getConnections() const
{
	return m_connections;
}

bool DAmnUser::hasSameName(const QString &name)
{
	return name.toLower() == m_name.toLower();
}

bool DAmnUser::setConnection(const DAmnConnection &connection)
{
	DAmnConnectionsIterator it = findConnection(connection.room);

	if (it != m_connections.end())
	{
		it->idle = connection.idle;
		it->online = connection.online;

		return false;
	}

	m_connections << connection;

	return true;
}

void DAmnUser::removeConnections()
{
	m_connections.clear();
}

DAmnConnectionsIterator DAmnUser::findConnection(const QString &name)
{
	DAmnConnectionsIterator it = m_connections.begin();

	while(it != m_connections.end())
	{
		if (it->room.toLower() == name.toLower()) break;

		++it;
	}

	return it;
}
