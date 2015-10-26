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

#ifndef UPDATER_H
#define UPDATER_H

#define UPDATE_URL "http://kervala.net/utils/update.php"

class Updater : public QObject
{
	Q_OBJECT

public:
	Updater(QObject *parent);
	virtual ~Updater();

	bool checkUpdates();

signals:
	void newVersionDetected(const QString &url, const QString &date, uint size, const QString &version);
	void noNewVersionDetected();

public slots:
	void onReply(QNetworkReply *reply);

private:
	QNetworkAccessManager *m_manager;
};

#endif
