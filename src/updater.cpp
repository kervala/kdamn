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
#include "updater.h"
#include "utils.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

Updater::Updater(QObject *parent):QObject(parent), m_manager(NULL)
{
}

Updater::~Updater()
{
}

void Updater::onReply(QNetworkReply *reply)
{
	QByteArray content = reply->readAll();

	// always delete QNetworkReply to avoid memory leaks
	reply->deleteLater();
	reply = NULL;

	// received software update
	QVariantMap map;

#ifdef USE_QT5
	QJsonParseError jsonError;
	QJsonDocument doc = QJsonDocument::fromJson(content, &jsonError);

	if (jsonError.error == QJsonParseError::NoError) map = doc.toVariant().toMap();

#else
	QScriptEngine engine;
	map = engine.evaluate("(" + QString(content) + ")").toVariant().toMap();
#endif

	int result = map["result"].toInt();

	if (result)
	{
		QString version = map["version"].toString();
		QString date = map["date"].toString();
		uint size = map["size"].toUInt();
		QString url = map["url"].toString();

		emit newVersionDetected(url, date, size, version);
	}
	else
	{
		emit noNewVersionDetected();
	}
}

bool Updater::checkUpdates()
{
	QString system;

#ifdef Q_OS_WIN32
	system = "win";
#ifdef Q_OS_WIN64
	system += "64";
#else
	system += "32";
#endif
#elif defined(Q_OS_OSX)
	system += "osx";
#endif

	if (system.isEmpty()) return false;

	if (!m_manager)
	{
		m_manager = new QNetworkAccessManager(this);
		connect(m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onReply(QNetworkReply*)));
	}

	QString url = QString("%1?system=%2&version=%3&app=%4").arg(UPDATE_URL).arg(system).arg(QApplication::applicationVersion()).arg(QApplication::applicationName());

	QNetworkRequest req;
	req.setUrl(QUrl(url));

#ifdef USE_QT5
	req.setHeader(QNetworkRequest::UserAgentHeader, GetUserAgent());
#else
	req.setRawHeader("User-Agent", GetUserAgent().toLatin1());
#endif

	QNetworkReply *reply = m_manager->get(req);

	return reply != NULL;
}
