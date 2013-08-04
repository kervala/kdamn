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
#include "oauth2.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

OAuth2* OAuth2::s_instance = NULL;

OAuth2::OAuth2(QObject *parent):QObject(parent), m_manager(NULL), m_clientId(0), m_expiresIn(0)
{
	if (s_instance == NULL) s_instance = this;

	m_clientId = 474;
	m_clientSecret = "6a8b3dacb0d41c5d177d6f189df772d1";

	m_manager = new QNetworkAccessManager(this);

	connect(m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onReply(QNetworkReply*)));
	connect(m_manager, SIGNAL(proxyAuthenticationRequired(QNetworkProxy, QAuthenticator *)), SLOT(onAuthentication(QNetworkProxy, QAuthenticator *)));
}

OAuth2::~OAuth2()
{
	s_instance = NULL;
}

bool OAuth2::get(const QString &url, const QString &referer)
{
	QNetworkRequest req;
	req.setUrl(QUrl(url));
//	req.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.2.8) Gecko/20100731 Firefox/3.6.8 (Swiftfox)");
	if (!referer.isEmpty()) req.setRawHeader("Referer", referer.toLatin1());

	QNetworkReply *reply = m_manager->get(req);

	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onReplyError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));

	return true;
}

bool OAuth2::requestAuthorization()
{
	return get(QString("https://www.deviantart.com/oauth2/draft15/authorize?response_type=code&client_id=%1&redirect_uri=kdamn://oauth2/login").arg(m_clientId));
}

bool OAuth2::authorizeApplication(bool authorize)
{
	QString siteurl("https://www.deviantart.com/settings/update-applications");

	QNetworkRequest req;
	req.setUrl(QUrl(siteurl));
//	req.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.2.8) Gecko/20100731 Firefox/3.6.8 (Swiftfox)");
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("client_id", QString::number(m_clientId));
	params.addQueryItem("terms_agree", "true");
	params.addQueryItem("authorize", authorize ? "true":"false");

	QByteArray data;

#ifdef USE_QT5
	data = params.query().toUtf8();
#else
	data = params.encodedQuery();
#endif

	QNetworkReply *reply = m_manager->post(req, data);

	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onReplyError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));

	return true;
}

bool OAuth2::login(const QString &login, const QString &password)
{
	m_login = login;

	QString siteurl("https://www.deviantart.com/join/oauth2");

	QNetworkRequest req;
	req.setUrl(QUrl(siteurl));
//	req.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.2.8) Gecko/20100731 Firefox/3.6.8 (Swiftfox)");
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("subdomain", "www");
	params.addQueryItem("oauth2", "1");
	params.addQueryItem("username", login);
	params.addQueryItem("password", password);

	QByteArray data;

#ifdef USE_QT5
	data = params.query().toUtf8();
#else
	data = params.encodedQuery();
#endif

	QNetworkReply *reply = m_manager->post(req, data);

	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onReplyError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));

	return true;
}

bool OAuth2::requestToken(const QString &code)
{
	return get(QString("https://www.deviantart.com/oauth2/draft15/token?client_id=%1&client_secret=%2&grant_type=%3&code=%4&refresh_token=%5").arg(m_clientId).arg(m_clientSecret).arg(m_token.isEmpty() ? "authorization_code":"refresh_token").arg(code).arg(m_refreshToken));
}

bool OAuth2::requestUserInfo()
{
	return get(QString("https://www.deviantart.com/api/draft15/user/whoami?access_token=%1").arg(m_token));
}

bool OAuth2::requestDAmnToken()
{
	return get(QString("https://www.deviantart.com/api/draft15/user/damntoken?access_token=%1").arg(m_token));
}

bool OAuth2::uploadToStash(const QString &filename)
{
	QString siteurl(QString("https://www.deviantart.com/api/draft15/stash/submit?access_token=%1").arg(m_token));

	QNetworkRequest req;
	req.setUrl(QUrl(siteurl));
//	req.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.2.8) Gecko/20100731 Firefox/3.6.8 (Swiftfox)");

	QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

//	QHttpPart textPart;
//	textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"text\""));
//	textPart.setBody("my text");

	QFile *file = new QFile(filename, multiPart);
	file->open(QIODevice::ReadOnly);

	QHttpPart imagePart;
	imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
	imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image\""));
	imagePart.setBodyDevice(file);

//	multiPart->append(textPart);
	multiPart->append(imagePart);

	QNetworkAccessManager manager;
	QNetworkReply *reply = manager.post(req, multiPart);
	multiPart->setParent(reply);

	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onReplyError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));

	return true;
}

QString OAuth2::getSupportedImageFormatsFilter()
{
	static QString s_imagesFilter;

	if (s_imagesFilter.isEmpty())
	{
		QList<QByteArray> formats = QImageReader::supportedImageFormats();

		foreach(const QByteArray &format, formats)
		{
			// some formats are not supported in QTextBrowser
			if (format == "mng") continue;

			if (!s_imagesFilter.isEmpty()) s_imagesFilter += "|";

			s_imagesFilter += format;
		}

		s_imagesFilter = "(" + s_imagesFilter + ")";
	}

	return s_imagesFilter;
}


bool OAuth2::loginSite(const QString &login, const QString &password)
{
	m_login = login;

	QString siteurl("https://www.deviantart.com/users/login");

	QNetworkRequest req;
	req.setUrl(QUrl(siteurl));
//	req.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.2.8) Gecko/20100731 Firefox/3.6.8 (Swiftfox)");
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	req.setRawHeader("Referer", "http://www.deviantart.com/users/rockedout");

#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("ref", siteurl);
	params.addQueryItem("username", login);
	params.addQueryItem("password", password);
	params.addQueryItem("remember_me", "1");

	QByteArray data;

#ifdef USE_QT5
	data = params.query().toUtf8();
#else
	data = params.encodedQuery();
#endif

	QNetworkReply *reply = m_manager->post(req, data);

	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onReplyError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));

	return true;
}

bool OAuth2::requestAuthToken()
{
	return get("http://chat.deviantart.com/chat/Botdom");
}

void OAuth2::onReply(QNetworkReply *reply)
{
	if (reply->error() == QNetworkReply::NoError)
	{
	}

	QString redirection = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl().toString();
	QString url = reply->url().toString();

	if (url.indexOf(QRegExp("\\." + OAuth2::getSupportedImageFormatsFilter() + "(\\?([0-9]+))?$")) > -1)
	{
		QByteArray content = reply->readAll();
		QString url = reply->url().toString();
		QString md5 = QCryptographicHash::hash(url.toLatin1(), QCryptographicHash::Md5).toHex();

		QString cachePath;

#ifdef USE_QT5
		cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
		cachePath = QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
#endif

		QFile file(QString("%1/%2").arg(QDir::fromNativeSeparators(cachePath)).arg(md5));

		if (file.open(QIODevice::WriteOnly))
		{
			file.write(content);
		}

		emit imageDownloaded(md5);
	}
	else if (url.indexOf("/whoami") > -1)
	{
		QByteArray json = reply->readAll();

#ifdef USE_QT5
		QJsonParseError error;
		QJsonDocument doc = QJsonDocument::fromJson(json, &error);

		if (error.error == QJsonParseError::NoError)
		{
			QJsonObject object = doc.object();

			QJsonObject::const_iterator it = object.constFind("username");

			if (it != object.constEnd())
			{
				m_login = it.value().toString();

				requestDAmnToken();
			}
			else
			{
				emit errorReceived(tr("No dAmn token received"));
			}
		}
		else
		{
			emit errorReceived(error.errorString());
		}
#else
		QScriptEngine engine;
		QScriptValue sc = engine.evaluate("(" + QString(json) + ")");

		m_login = sc.property("username").toString();

		if (!m_login.isEmpty())
		{
			requestDAmnToken();
		}
		else
		{
			emit errorReceived(tr("No login received"));
		}
#endif
	}
	else if (url.indexOf("/damntoken") > -1)
	{
		QByteArray json = reply->readAll();

#ifdef USE_QT5
		QJsonParseError error;
		QJsonDocument doc = QJsonDocument::fromJson(json, &error);

		if (error.error == QJsonParseError::NoError)
		{
			QJsonObject object = doc.object();

			QJsonObject::const_iterator it = object.constFind("damntoken");

			if (it != object.constEnd())
			{
				m_damnToken = it.value().toString();

				emit damnTokenReceived(m_login, m_damnToken);
			}
			else
			{
				emit errorReceived(tr("No dAmn token received"));
			}
		}
		else
		{
			emit errorReceived(error.errorString());
		}
#else
		QScriptEngine engine;
		QScriptValue sc = engine.evaluate("(" + QString(json) + ")");

		m_damnToken = sc.property("damntoken").toString();

		if (!m_damnToken.isEmpty())
		{
			emit damnTokenReceived(m_login, m_damnToken);
		}
		else
		{
			emit errorReceived(tr("No dAmn token received"));
		}
#endif
	}
	else if (url.indexOf("/token") > -1)
	{
		QByteArray json = reply->readAll();
		QString status;

#ifdef USE_QT5
		QJsonParseError error;
		QJsonDocument doc = QJsonDocument::fromJson(json, &error);

		if (error.error == QJsonParseError::NoError)
		{
			QJsonObject object = doc.object();

			QJsonObject::const_iterator it;

			it = object.constFind("status");

			if (it != object.constEnd())
			{
				status = it.value().toString();

				if (status == "success")
				{
					// seconds number before a session expire
					it = object.constFind("expires_in");

					if (it != object.constEnd()) m_expiresIn = (int)it.value().toDouble();

					it = object.constFind("access_token");

					if (it != object.constEnd()) m_token = it.value().toString();

					it = object.constFind("refresh_token");

					if (it != object.constEnd()) m_refreshToken = it.value().toString();
				}
				else
				{
					emit errorReceived(status);
				}
			}
			else
			{
				emit errorReceived(tr("No JSON status received"));
			}
		}
#else
		QScriptEngine engine;
		QScriptValue sc = engine.evaluate("(" + QString(json) + ")");

		status = sc.property("status").toString();

		if (status == "success")
		{
			m_expiresIn = sc.property("expires_in").toInt32();
			m_token = sc.property("access_token").toString();
			m_refreshToken = sc.property("access_token").toString();
		}
#endif

		if (status == "success")
		{
			if (m_login.indexOf('@') > -1)
			{
				// we have an email, we need to request the login
				requestUserInfo();
			}
			else
			{
				// login is good, request DAmn token
				requestDAmnToken();
			}
		}
		else
		{
			emit errorReceived(status);
		}
	}
	else if (url.indexOf("/oauth2") > -1 && redirection.indexOf("/blank") > -1)
	{
		// login successful, authorize the application now
		requestAuthorization();
	}
	else if (url.indexOf("/authorize") > -1 && redirection.indexOf("/applications") > -1)
	{
		get(redirection, url);
	}
	else if (redirection.startsWith("kdamn://"))
	{
		QRegExp reg("^kdamn://oauth2/login\\?code=([0-9]+)$");

		if (reg.indexIn(redirection) > -1)
		{
			QString code = reg.cap(1);

			requestToken(code);
		}
	}
	else if (redirection.indexOf("/login") > -1)
	{
		requestAuthToken();
	}
	else if (url.indexOf("/Botdom") > -1)
	{
		QRegExp reg("dAmn_Login\\( \"([A-Za-z0-9_-]+)\", \"([a-f0-9]{32})\"");

		QString html = reply->readAll();

		if (reg.indexIn(html) > -1)
		{
			m_login = reg.cap(1);
			m_damnToken = reg.cap(2);

			emit damnTokenReceived(m_login, m_damnToken);
		}
	}
	else if (url.indexOf("/applications") > -1)
	{
		authorizeApplication(true);
	}
	else
	{
		QString html = reply->readAll();

		qDebug() << html;
	}

	reply->deleteLater();
}

void OAuth2::onReplyError(QNetworkReply::NetworkError error)
{
	qDebug() << "Error:" << error;

	emit errorReceived(tr("Network error: %1").arg(error));
}

void OAuth2::onSslErrors(const QList<QSslError> &errors)
{
	qDebug() << "SSL Errors:" << errors;

	foreach(const QSslError &error, errors)
	{
		emit errorReceived(tr("SSL errors: %1").arg(error.errorString()));
	}
}

void OAuth2::onAuthentication(const QNetworkProxy &proxy, QAuthenticator *auth)
{
	qDebug() << "auth";

	emit errorReceived(tr("Authentication required"));
}
