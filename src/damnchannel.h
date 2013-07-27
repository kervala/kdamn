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

#ifndef DAMNCHANNEL_H
#define DAMNCHANNEL_H

struct DAmnChannelUser
{
	QString name;
	QString pc;
	QString by;
	int count;

	DAmnChannelUser()
	{
		count = 0;
	}

	bool operator == (const DAmnChannelUser &other)
	{
		return other.name.toLower() == name.toLower();
	}
};

typedef QList<DAmnChannelUser> DAmnChannelUsers;
typedef DAmnChannelUsers::iterator DAmnChannelUsersIterator;

struct DAmnPrivClass
{
	int id;
	QString name;
};

typedef QList<DAmnPrivClass> DAmnPrivClasses;
typedef DAmnPrivClasses::iterator DAmnPrivClassesIterator;

struct DAmnChannelProperty
{
	QString name;
	QString html;
	QString text;
	QString by;
	QString ts;
};

typedef QList<DAmnChannelProperty> DAmnChannelProperties;
typedef DAmnChannelProperties::iterator DAmnChannelPropertiesIterator;

class DAmnChannel : public QObject
{
	Q_OBJECT

public:
	DAmnChannel(const QString &name, QObject *parent);
	virtual ~DAmnChannel();

	void setName(const QString &name);
	QString getName() const;

	bool hasSameName(const QString &name);

	void setProperty(const DAmnChannelProperty &prop);
	bool getProperty(const QString &name, DAmnChannelProperty &prop) const;

	bool addUser(const QString &name);
	bool addUser(const DAmnChannelUser &user);

	bool removeUser(const QString &name);
	void removeUsers();

	bool setUser(const DAmnChannelUser &user);

	void removePrivClasses();
	bool setPrivClass(const DAmnPrivClass &priv);

private:
	DAmnChannelUsersIterator findUser(const QString &name);
	DAmnPrivClassesIterator findPrivClass(const QString &name);

	QString m_name;

	DAmnChannelProperties m_properties;
	DAmnPrivClasses m_privclasses;
	DAmnChannelUsers m_users;
};

#endif
