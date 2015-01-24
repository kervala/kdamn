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
#include "oauth2.h"
#include "cookies.h"
#include "utils.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

bool OAuth2::uploadToStash(const QStringList &filenames, const QString &room)
{
	foreach(const QString &filename, filenames)
	{
		StashFile file;
		file.filename = filename;
		file.room = room;

		if (m_filesToUpload.indexOf(file) < 0)
		{
			m_filesToUpload.push_back(file);
		}
	}

	// only check access token for the first file
	return m_filesToUpload.size() > 0 && requestPlacebo();
}

QString OAuth2::getAuthorizationUrl() const
{
	return QString("%1?response_type=code&client_id=%2&redirect_uri=%3&scope=basic").arg(AUTHORIZE_URL).arg(m_clientId).arg(REDIRECT_APP);
}

bool OAuth2::requestAuthorization()
{
	return get(getAuthorizationUrl(), OAUTH2LOGIN_URL);
}

bool OAuth2::requestAccessToken(const QString &code)
{
	QString query;

	if (!code.isEmpty())
	{
		query = QString("authorization_code&code=%1&redirect_uri=%2").arg(code).arg(REDIRECT_APP);
	}
	else
	{
		if (m_refreshToken.isEmpty()) return requestAuthorization();

		m_accessToken.clear();

		query = QString("refresh_token&refresh_token=%1").arg(m_refreshToken);
	}

	return get(QString("%1?client_id=%2&client_secret=%3&grant_type=%4").arg(OAUTH2_TOKEN_URL).arg(m_clientId).arg(m_clientSecret).arg(query));
}

bool OAuth2::requestPlacebo()
{
	if (m_accessToken.isEmpty())
	{
		m_actions.push_front(ActionRequestPlacebo);

		return requestAccessToken();
	}

	return get(QString("%1/placebo?access_token=%2").arg(OAUTH2_URL).arg(m_accessToken));
}

bool OAuth2::requestUserInfo()
{
	if (m_accessToken.isEmpty()) return false;

	return get(QString("%1/user/whoami?access_token=%2").arg(OAUTH2_URL).arg(m_accessToken));
}

bool OAuth2::requestDAmnToken()
{
	// don't request dAmn token if already got
	if (!m_damnToken.isEmpty()) return true;

	if (m_accessToken.isEmpty())
	{
		m_actions.push_front(ActionRequestDAmnToken);

		return requestAccessToken();
	}

	return get(QString("%1/user/damntoken?access_token=%2").arg(OAUTH2_URL).arg(m_accessToken));
}

bool OAuth2::requestStash(const QString &filename, const QString &room)
{
	if (m_accessToken.isEmpty()) return false;

	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly))
	{
		emit errorReceived(tr("Unable to read file %1").arg(filename));

		return false;
	}

	QFileInfo info(filename);

	// check mime type
	QMimeDatabase mimeDatabase;
	QMimeType mimeType = mimeDatabase.mimeTypeForFile(info, QMimeDatabase::MatchContent);
	QString mime = mimeType.name();

	QNetworkRequest req;
	req.setUrl(QUrl(QString("%1/stash/submit?access_token=%2").arg(OAUTH2_URL).arg(m_accessToken)));
	addUserAgent(req);

	QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType, this);

	QHttpPart folderPart;
	folderPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"folder\"");
	folderPart.setBody(QString("kdamn_%1").arg(room).toUtf8());

	QString title = info.baseName().left(50);

	QHttpPart titlePart;
	titlePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"title\"");
	titlePart.setBody(title.toUtf8());

	QHttpPart imagePart;
	imagePart.setHeader(QNetworkRequest::ContentTypeHeader, mime);
	imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QString("form-data; name=\"image\"; filename=\"%1\"").arg(info.fileName()));
	imagePart.setBody(file.readAll());

	multiPart->append(folderPart);
	multiPart->append(titlePart);
	multiPart->append(imagePart);

	QNetworkReply *reply = m_manager->post(req, multiPart);

	connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this, SIGNAL(uploadProgress(qint64, qint64)));
	connect(reply, SIGNAL(finished()), this, SLOT(onUploadFinished()));

	emit uploadProgress(0, info.size());

	return true;
}

void OAuth2::onUploadFinished()
{
	emit uploadProgress(0, 0);
}

bool OAuth2::checkUpdates()
{
	QString system;

#ifdef Q_OS_WIN32
	system = "win";
#ifdef _WIN64
	system += "64";
#else
	system += "32";
#endif
#elif defined(Q_OS_OSX)
	system += "osx";
#endif

	return !system.isEmpty() ? get(QString("%1?system=%2&version=%3&app=%4").arg(UPDATE_URL).arg(system).arg(QApplication::applicationVersion()).arg(QApplication::applicationName())):false;
}

void OAuth2::processJson(const QByteArray &content, const QString &path)
{
	QVariantMap map;

	// we received JSON response
#ifdef USE_QT5
	QJsonParseError jsonError;
	QJsonDocument doc = QJsonDocument::fromJson(content, &jsonError);

	if (jsonError.error != QJsonParseError::NoError)
	{
		emit errorReceived(jsonError.errorString());
		return;
	}

	map = doc.toVariant().toMap();
#else
	QScriptEngine engine;
	map = engine.evaluate("(" + QString(content) + ")").toVariant().toMap();
#endif

	QString status = map["status"].toString();
	QString error = map["error"].toString();
	QString errorDescription = map["error_description"].toString();

	if (path.endsWith("/whoami"))
	{
		m_login = map["username"].toString();

		if (!m_login.isEmpty())
		{
			requestDAmnToken();
		}
		else
		{
			emit errorReceived(tr("No login received"));
		}
	}
	else if (path.endsWith("/placebo"))
	{
		if (status == "success")
		{
			processNextUpload();
		}
		else if (error == "expired_token" || error == "invalid_token")
		{
			// we are fixing the error ourself
			status.clear();

			requestAccessToken();
		}
	}
	else if (path.endsWith("/user/damntoken"))
	{
		if (error == "invalid_token")
		{
			// we are fixing the error ourself
			status.clear();

			// clear previous dAmn token
			m_accessToken.clear();

			// request a new dAmn token
			requestDAmnToken();
		}
		else
		{
			m_damnToken = map["damntoken"].toString();

			if (!m_damnToken.isEmpty())
			{
				emit damnTokenReceived(m_login, m_damnToken);
			}
			else
			{
				emit errorReceived(tr("No dAmn token received"));
			}

			processNextAction();
		}
	}
	else if (path.endsWith("/token"))
	{
		if (status == "success")
		{
			// seconds number before a session expire
			m_expiresIn = (int)map["expires_in"].toDouble();
			m_accessToken = map["access_token"].toString();
			m_refreshToken = map["access_token"].toString();

			m_lastAccessTokenTime = QDateTime::currentDateTime();

			emit accessTokenReceived(m_accessToken, m_refreshToken);

			if (m_login.indexOf('@') > -1)
			{
				// we have an email, we need to request the login
				requestUserInfo();
			}
			else
			{
				processNextAction();
			}
		}
		else if (error == "invalid_grant" || error == "invalid_request")
		{
			// we are fixing the error ourself
			status.clear();

			// refresh token invalid
			m_refreshToken.clear();

			// clear cookies
			qobject_cast<Cookies*>(m_manager->cookieJar())->clear();

			requestAuthorization();
		}
	}
	else if (path.endsWith("/stash/submit"))
	{
		if (status == "success")
		{
			qint64 stashId = (qint64)map["stashid"].toDouble();
			QString folder = map["folder"].toString();
			qint64 folderId = (qint64)map["folderid"].toDouble();

			if (!m_filesToUpload.isEmpty())
			{
				int index = -1;

				for(int i = 0; i < m_filesToUpload.size(); ++i)
				{
					if (m_filesToUpload[i].status == StashFile::StatusUploading)
					{
						index = i;
						break;
					}
				}

				if (index > -1)
				{
					StashFile file = m_filesToUpload[index];

					m_filesToUpload.removeAt(index);

					QString stashUrl = QString("http://sta.sh/0%1").arg(base36enc(stashId));

					emit imageUploaded(file.room, stashUrl);
				}
				else
				{
					qDebug() << "File not found in stash";
				}
			}

			processNextUpload();
		}
		else
		{
			qCritical() << "Error while uploading file";
		}
	}
	else
	{
		qWarning() << "JSON data not processed" << path << content;
	}

	if (status == "error")
	{
		emit errorReceived(tr("API error: %1").arg(errorDescription.isEmpty() ? error:errorDescription));
	}
}

void OAuth2::processNewVersions(const QByteArray &content)
{
	QVariantMap map;

#ifdef USE_QT5
	QJsonParseError jsonError;
	QJsonDocument doc = QJsonDocument::fromJson(content, &jsonError);

	if (jsonError.error != QJsonParseError::NoError)
	{
		emit errorReceived(jsonError.errorString());
		return;
	}

	map = doc.toVariant().toMap();
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
}
