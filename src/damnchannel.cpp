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
#include "damnchannel.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

DAmnChannel::DAmnChannel(const QString &name, QObject *parent):QObject(parent), m_name(name)
{
}

DAmnChannel::~DAmnChannel()
{
}

void DAmnChannel::setName(const QString &name)
{
	m_name = name;
}

QString DAmnChannel::getName() const
{
	return m_name;
}

bool DAmnChannel::hasSameName(const QString &name)
{
	return name.toLower() == m_name.toLower();
}

void DAmnChannel::setProperty(const DAmnChannelProperty &prop)
{
	QList<DAmnChannelProperty>::iterator it = m_properties.begin();

	while(it != m_properties.end())
	{
		if (it->name.toLower() == prop.name.toLower())
		{
			it->by = prop.by;
			it->ts = prop.ts;
			it->text = prop.text;
			it->html = prop.html;
			return;
		}

		++it;
	}

	m_properties << prop;
}

bool DAmnChannel::getProperty(const QString &name, DAmnChannelProperty &prop) const
{
	foreach(const DAmnChannelProperty &p, m_properties)
	{
		if (p.name.toLower() == name.toLower())
		{
			prop = p;
			return true;
		}
	}

	return false;
}

bool DAmnChannel::addUser(const QString &name)
{
	DAmnChannelUser user;
	user.name = name;
	user.count = 1;

	return addUser(user);
}

bool DAmnChannel::addUser(const DAmnChannelUser &user)
{
	DAmnChannelUsersIterator it = findUser(user.name);

	if (it != m_users.end())
	{
		++it->count;

		// set some variables if defined
		if (it->by.isEmpty()) it->by = user.by;
		if (it->pc.isEmpty()) it->pc = user.pc;

		return false;
	}

	m_users << user;

	return true;
}

bool DAmnChannel::removeUser(const QString &name)
{
	DAmnChannelUsersIterator it = findUser(name);

	if (it == m_users.end()) return false;

	if (--it->count >= 0) return false;

	m_users.erase(it);

	return true;
}

void DAmnChannel::removeUsers()
{
	m_users.clear();
}

bool DAmnChannel::setUser(const DAmnChannelUser &user)
{
	DAmnChannelUsersIterator it = findUser(user.name);

	if (it == m_users.end()) return false;

	it->by = user.by;
	it->pc = user.pc;

	return true;
}

void DAmnChannel::removePrivClasses()
{
	m_privclasses.clear();
}

bool DAmnChannel::setPrivClass(const DAmnPrivClass &priv)
{
	DAmnPrivClassesIterator it = findPrivClass(priv.name);

	if (it != m_privclasses.end())
	{
		it->id = priv.id;

		return false;
	}

	m_privclasses << priv;

	return true;
}

DAmnChannelUsersIterator DAmnChannel::findUser(const QString &name)
{
	DAmnChannelUsersIterator it = m_users.begin();

	while(it != m_users.end())
	{
		if (it->name.toLower() == name.toLower()) break;

		++it;
	}

	return it;
}

DAmnPrivClassesIterator DAmnChannel::findPrivClass(const QString &name)
{
	DAmnPrivClassesIterator it = m_privclasses.begin();

	while(it != m_privclasses.end())
	{
		if (it->name.toLower() == name.toLower()) break;

		++it;
	}

	return it;
}
