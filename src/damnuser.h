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

#ifndef DAMNUSER_H
#define DAMNUSER_H

struct DAmnConnection
{
	QString room;
	int online;
	int idle;
};

typedef QList<DAmnConnection> DAmnConnections;
typedef DAmnConnections::iterator DAmnConnectionsIterator;

class DAmnUser : public QObject
{
	Q_OBJECT

public:
	DAmnUser(const QString &name, QObject *parent);
	virtual ~DAmnUser();

	QString getName() const;
	void setName(const QString &name);

	int getUserIcon() const;
	void setUserIcon(int usericon);

	QChar getSymbol() const;
	void setSymbol(const QChar &symbol);

	QString getRealName() const;
	void setRealName(const QString &realname);

	QString getGPC() const;
	void setGPC(const QString &gpc);

	DAmnConnections getConnections() const;

	bool hasSameName(const QString &name);

	bool setConnection(const DAmnConnection &connection);
	void removeConnections();

private:
	DAmnConnectionsIterator findConnection(const QString &name);

	QString m_name;
	int m_usericon;
	QChar m_symbol;
	QString m_realname;
	QString m_gpc;

	DAmnConnections m_connections;
};

#endif
