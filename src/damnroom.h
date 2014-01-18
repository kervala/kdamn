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

#ifndef DAMNROOM_H
#define DAMNROOM_H

struct DAmnRoomUser
{
	QString name;
	QString pc;
	QString by;
	int count;

	DAmnRoomUser()
	{
		count = 0;
	}

	bool operator == (const DAmnRoomUser &other)
	{
		return other.name.toLower() == name.toLower();
	}
};

typedef QList<DAmnRoomUser> DAmnRoomUsers;
typedef DAmnRoomUsers::iterator DAmnRoomUsersIterator;

struct DAmnPrivClass
{
	int id;
	QString name;
};

typedef QList<DAmnPrivClass> DAmnPrivClasses;
typedef DAmnPrivClasses::iterator DAmnPrivClassesIterator;

struct DAmnRoomProperty
{
	QString name;
	QString html;
	QString text;
	QString by;
	QString ts;
};

typedef QList<DAmnRoomProperty> DAmnRoomProperties;
typedef DAmnRoomProperties::iterator DAmnRoomPropertiesIterator;

class DAmnRoom : public QObject
{
	Q_OBJECT

public:
	DAmnRoom(const QString &name, QObject *parent);
	virtual ~DAmnRoom();

	void setName(const QString &name);
	QString getName() const;

	bool hasSameName(const QString &name);

	void setProperty(const DAmnRoomProperty &prop);
	bool getProperty(const QString &name, DAmnRoomProperty &prop) const;

	bool addUser(const QString &name);
	bool addUser(const DAmnRoomUser &user);

	bool removeUser(const QString &name);
	void removeUsers();

	bool setUser(const DAmnRoomUser &user);

	void removePrivClasses();
	bool setPrivClass(const DAmnPrivClass &priv);

private:
	DAmnRoomUsersIterator findUser(const QString &name);
	DAmnPrivClassesIterator findPrivClass(const QString &name);

	QString m_name;

	DAmnRoomProperties m_properties;
	DAmnPrivClasses m_privclasses;
	DAmnRoomUsers m_users;
};

#endif
