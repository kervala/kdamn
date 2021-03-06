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
#include "cookies.h"
#include "configfile.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

Cookies::Cookies(QObject *parent):QNetworkCookieJar(parent)
{
}

Cookies::~Cookies()
{
}

void Cookies::clear()
{
	QList<QNetworkCookie> cookies;

	setAllCookies(cookies);
}

void Cookies::dump()
{
	QList<QNetworkCookie> cookies = allCookies();

	foreach(const QNetworkCookie &cookie, cookies)
	{
		qDebug() << cookie.name() << "=" << cookie.value();
	}
}

QString Cookies::get(const QString &name) const
{
	QList<QNetworkCookie> cookies = allCookies();

	foreach(const QNetworkCookie &cookie, cookies)
	{
		if (cookie.name() == name) return cookie.value();
	}

	return QString();
}

void Cookies::removeExpiredCookies(QList<QNetworkCookie> &cookies)
{
    QDateTime now = QDateTime::currentDateTime();

    for (int i = cookies.count() - 1; i >= 0; --i)
	{
		if (cookies.at(i).expirationDate() < now) cookies.removeAt(i);
	}
}
