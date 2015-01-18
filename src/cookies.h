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

#ifndef COOKIES_H
#define COOKIES_H

class Cookies : public QNetworkCookieJar
{
	Q_OBJECT

public:
	Cookies(QObject *parent);
	virtual ~Cookies();

	void clear();
	void dump();

	QString get(const QString &name) const;

protected:
	void removeExpiredCookies(QList<QNetworkCookie> &cookies);
};

#endif
