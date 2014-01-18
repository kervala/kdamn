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

#include "common.h"
#include "oauth2.h"
#include "cookies.h"
#include "utils.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

static QString base36enc(qint64 value)
{
	static const QString base36("0123456789abcdefghijklmnopqrstuvwxyz");

	QString res;

	do
	{
		res.prepend(base36[(int)(value % 36)]);
	}
	while (value /= 36);

	return res;
}

QString OAuth2::s_userAgent;
OAuth2* OAuth2::s_instance = NULL;

OAuth2::OAuth2(QObject *parent):QObject(parent), m_manager(NULL), m_clientId(0), m_expiresIn(0)
{
	if (s_instance == NULL) s_instance = this;

	m_clientId = 474;
	m_clientSecret = "6a8b3dacb0d41c5d177d6f189df772d1";

	InitSystemProgress();
}

OAuth2::~OAuth2()
{
	s_instance = NULL;

	UninitSystemProgress();
}

void OAuth2::init()
{
	if (m_manager) return;

	m_manager = new QNetworkAccessManager(this);
	m_manager->setCookieJar(new Cookies(m_manager));

	connect(m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onReply(QNetworkReply*)));
	connect(m_manager, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator *)), this, SLOT(onAuthentication(QNetworkReply*, QAuthenticator *)));
	connect(m_manager, SIGNAL(proxyAuthenticationRequired(QNetworkProxy, QAuthenticator *)), SLOT(onProxyAuthentication(QNetworkProxy, QAuthenticator *)));
}

void OAuth2::addUserAgent(QNetworkRequest &req) const
{
	if (s_userAgent.isEmpty()) return;

#ifdef USE_QT5
	req.setHeader(QNetworkRequest::UserAgentHeader, s_userAgent);
#else
	req.setRawHeader("User-Agent", s_userAgent.toLatin1());
#endif
}

bool OAuth2::get(const QString &url, const QString &referer)
{
	init();

	QNetworkRequest req;
	req.setUrl(QUrl(url));

	addUserAgent(req);

	if (!referer.isEmpty()) req.setRawHeader("Referer", referer.toLatin1());

	QNetworkReply *reply = m_manager->get(req);

	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onReplyError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));

	return true;
}

bool OAuth2::post(const QString &url, const QByteArray &data, const QString &referer)
{
	init();

	QNetworkRequest req;
	req.setUrl(QUrl(url));
	addUserAgent(req);
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	if (!referer.isEmpty()) req.setRawHeader("Referer", referer.toLatin1());

	QNetworkReply *reply = m_manager->post(req, data);

	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onReplyError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));

	return true;
}

bool OAuth2::authorizeApplication(bool authorize)
{
#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("client_id", QString::number(m_clientId));
	params.addQueryItem("response_type", "code");
	params.addQueryItem("redirect_uri", "kdamn://oauth2/login");
	params.addQueryItem("scope", "basic");
	params.addQueryItem("state", "");
	params.addQueryItem("authorized", authorize ? "1":"");
	params.addQueryItem("terms_agree[]", "1");
	params.addQueryItem("terms_agree[]", "0");

	QByteArray data;

#ifdef USE_QT5
	data = params.query().toUtf8();
#else
	data = params.encodedQuery();
#endif

	return post("https://www.deviantart.com/settings/authorize_app", data);
}

bool OAuth2::login(bool oauth2)
{
	init();

	if (oauth2)
	{
		// try to reuse an access token
		if (!m_accessToken.isEmpty()) return requestPlacebo();

		// try to reuse previous cookies
		if (!m_manager->cookieJar()->cookiesForUrl(QUrl("https://www.deviantart.com/")).isEmpty()) return requestAuthorization();

		return loginOAuth2();
	}
		
	return getValidateToken();
}

bool OAuth2::uploadToStash(const QString &filename, const QString &room)
{
	if (m_accessToken.isEmpty())
	{
		emit errorReceived(tr("No access token, please use the OAuth2 login method"));

		return false;
	}

	StashFile file;
	file.filename = filename;
	file.room = room;

	if (m_filesToUpload.indexOf(file) < 0) m_filesToUpload.push_back(file);

	// only check access token for the first file
	return m_filesToUpload.size() == 1 ? requestPlacebo():true;
}

bool OAuth2::loginOAuth2()
{
#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("subdomain", "www");
	params.addQueryItem("oauth2", "1");
	params.addQueryItem("username", m_login);
	params.addQueryItem("password", m_password);

	QByteArray data;

#ifdef USE_QT5
	data = params.query().toUtf8();
#else
	data = params.encodedQuery();
#endif

	return post("https://www.deviantart.com/join/oauth2", data);
}

bool OAuth2::requestAuthorization()
{
	return get(QString("https://www.deviantart.com/oauth2/authorize?response_type=code&client_id=%1&redirect_uri=kdamn://oauth2/login").arg(m_clientId));
}

bool OAuth2::requestToken(const QString &code)
{
	QString query;
	
	if (!code.isEmpty())
	{
		query = QString("authorization_code&code=%1").arg(code);
	}
	else
	{
		m_accessToken.clear();
		query = QString("refresh_token&refresh_token=%1").arg(m_refreshToken);
	}

	return get(QString("https://www.deviantart.com/oauth2/token?client_id=%1&client_secret=%2&grant_type=%3").arg(m_clientId).arg(m_clientSecret).arg(query));
}

bool OAuth2::requestPlacebo()
{
	return get(QString("https://www.deviantart.com/api/oauth2/placebo?access_token=%1").arg(m_accessToken));
}

bool OAuth2::requestUserInfo()
{
	return get(QString("https://www.deviantart.com/api/oauth2/user/whoami?access_token=%1").arg(m_accessToken));
}

bool OAuth2::requestDAmnToken()
{
	// don't request dAmn token if already got
	if (!m_damnToken.isEmpty()) return true;

	return get(QString("https://www.deviantart.com/api/oauth2/user/damntoken?access_token=%1").arg(m_accessToken));
}

bool OAuth2::requestStash(const QString &filename, const QString &room)
{
	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly))
	{
		emit errorReceived(tr("Unable to read file %1").arg(filename));

		return false;
	}

	QFileInfo info(filename);

	QString mime;
	QString ext = info.suffix().toLower();

	if (ext == "png") mime = "image/png";
	else if (ext == "jpg" || ext == "jpeg") mime = "image/jpeg";
	else if (ext == "gif") mime = "image/gif";
	else mime = "application/binary";

	QNetworkRequest req;
	req.setUrl(QUrl(QString("https://www.deviantart.com/api/oauth2/stash/submit?access_token=%1").arg(m_accessToken)));
	addUserAgent(req);

	QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType, this);

	QHttpPart folderPart;
	folderPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"folder\"");
	folderPart.setBody(QString("kdamn_%1").arg(room).toUtf8());

	QHttpPart titlePart;
	titlePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"title\"");
	titlePart.setBody(info.baseName().toUtf8());

	QHttpPart imagePart;
	imagePart.setHeader(QNetworkRequest::ContentTypeHeader, mime);
	imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QString("form-data; name=\"image\"; filename=\"%1\"").arg(info.fileName()));
	imagePart.setBody(file.readAll());

	multiPart->append(folderPart);
	multiPart->append(titlePart);
	multiPart->append(imagePart);

	QNetworkReply *reply = m_manager->post(req, multiPart);

	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onReplyError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));
	connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this, SLOT(onUploadProgress(qint64, qint64)));
	connect(reply, SIGNAL(finished()), this, SLOT(onUploadFinished()));

	BeginSystemProgress();

	return true;
}

void OAuth2::onUploadProgress(qint64 readBytes, qint64 totalBytes)
{
	UpdateSystemProgress(readBytes, totalBytes);
}

void OAuth2::onUploadFinished()
{
	EndSystemProgress();
}

QString OAuth2::getSupportedImageFormatsFilter()
{
	static QString s_imagesFilter;

	if (s_imagesFilter.isEmpty())
	{
		QList<QByteArray> formats = QImageReader::supportedImageFormats();

		foreach(const QByteArray &format, formats)
		{
			if (!s_imagesFilter.isEmpty()) s_imagesFilter += "|";

			s_imagesFilter += format;
		}

		s_imagesFilter = "(" + s_imagesFilter + ")";
	}

	return s_imagesFilter;
}

QString OAuth2::getUserAgent()
{
	if (s_userAgent.isEmpty())
	{
		QString system;
#ifdef Q_OS_WIN32
		system = "Windows ";

		switch (QSysInfo::WindowsVersion)
		{
			case QSysInfo::WV_32s: system += "3.1 with Win32s"; break;
			case QSysInfo::WV_95: system += "95"; break;
			case QSysInfo::WV_98: system += "98"; break;
			case QSysInfo::WV_Me: system += "Me"; break;
			case QSysInfo::WV_DOS_based: system += "DOS"; break;

			case QSysInfo::WV_4_0: system += "NT 4.0"; break;
			case QSysInfo::WV_5_0: system += "NT 5.0"; break;
			case QSysInfo::WV_5_1: system += "NT 5.1"; break;
			case QSysInfo::WV_5_2: system += "NT 5.2"; break;
			case QSysInfo::WV_6_0: system += "NT 6.0"; break;
			case QSysInfo::WV_6_1: system += "NT 6.1"; break;
			case QSysInfo::WV_6_2: system += "NT 6.2"; break;
			case QSysInfo::WV_NT_based: system += "NT"; break;

			case QSysInfo::WV_CE: system += "CE"; break;
			case QSysInfo::WV_CENET: system += "CE Net"; break;
			case QSysInfo::WV_CE_5: system += "CE 5"; break;
			case QSysInfo::WV_CE_6: system += "CE 6"; break;
			case QSysInfo::WV_CE_based: system += "CE"; break;
		}

		system += "; ";

		// Windows target processor
		system += QString("Win%1").arg(IsOS64bits() ? 64:32);

		system += "; ";

		// application target processor
#ifdef _WIN64
		system += "x64; ";
#else
		system += "i386;";
#endif

		system += QLocale::system().name().replace('_', '-');
#else
#endif
		s_userAgent = QString("%1/%2 (%3)").arg(QApplication::applicationName()).arg(QApplication::applicationVersion()).arg(system);
	}

	return s_userAgent;
}

bool OAuth2::getValidateToken()
{
	return get("https://www.deviantart.com/users/rockedout");
}

bool OAuth2::loginSite(const QString &validationToken, const QString &validationKey)
{
#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("ref", "");
	params.addQueryItem("username", m_login);
	params.addQueryItem("password", m_password);
	params.addQueryItem("remember_me", "1");
	params.addQueryItem("action", "Login");
	params.addQueryItem("validate_token", validationToken);
	params.addQueryItem("validate_key", validationKey);

	QByteArray data;

#ifdef USE_QT5
	data = params.query().toUtf8();
#else
	data = params.encodedQuery();
#endif

	return post("https://www.deviantart.com/users/login", data, "https://www.deviantart.com/users/rockedout");
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

	qobject_cast<Cookies*>(m_manager->cookieJar())->saveToDisk();

	QString redirection = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl().toString();
	QString url = reply->url().toString();

#ifdef USE_QT5
	QUrlQuery query(reply->url().query());
#else
	QUrl query = reply->url();
#endif

	bool isJson = query.hasQueryItem("access_token") || query.hasQueryItem("grant_type");
	QString path = reply->url().path();

	reply->deleteLater();

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
	else if (isJson)
	{
		// we received JSON response
		QByteArray json = reply->readAll();

		QString status, error, errorDescription;

#ifdef USE_QT5
		QJsonParseError jsonError;
		QJsonDocument doc = QJsonDocument::fromJson(json, &jsonError);

		if (jsonError.error != QJsonParseError::NoError)
		{
			emit errorReceived(jsonError.errorString());
			return;
		}

		QJsonObject object = doc.object();
		QJsonObject::const_iterator it;

		it = object.constFind("status");
		if (it != object.constEnd()) status = it.value().toString();

		it = object.constFind("error");
		if (it != object.constEnd()) error = it.value().toString();

		it = object.constFind("error_description");
		if (it != object.constEnd()) errorDescription = it.value().toString();
#else
		QScriptEngine engine;
		QScriptValue sc = engine.evaluate("(" + QString(json) + ")");

		status = sc.property("status").toString();
		error = sc.property("error").toString();
#endif

		if (path.endsWith("/whoami"))
		{
#ifdef USE_QT5
			it = object.constFind("username");

			if (it != object.constEnd()) m_login = it.value().toString();
#else
			m_login = sc.property("username").toString();
#endif

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
				if (!m_filesToUpload.isEmpty())
				{
					StashFile file = m_filesToUpload.front();

					requestStash(file.filename, file.room);
				}
				else if (m_damnToken.isEmpty())
				{
					requestDAmnToken();
				}
			}
			else if (error == "expired_token" || error == "invalid_token")
			{
				// we are fixing the error ourself
				status.clear();

				requestToken();
			}
		}
		else if (path.endsWith("/user/damntoken"))
		{
#ifdef USE_QT5
			it = object.constFind("damntoken");

			if (it != object.constEnd()) m_damnToken = it.value().toString();
#else
			m_damnToken = sc.property("damntoken").toString();
#endif

			if (!m_damnToken.isEmpty())
			{
				emit damnTokenReceived(m_login, m_damnToken);
			}
			else
			{
				emit errorReceived(tr("No dAmn token received"));
			}
		}
		else if (path.endsWith("/token"))
		{
			if (status == "success")
			{
#ifdef USE_QT5
				// seconds number before a session expire
				it = object.constFind("expires_in");
				if (it != object.constEnd()) m_expiresIn = (int)it.value().toDouble();

				it = object.constFind("access_token");
				if (it != object.constEnd()) m_accessToken = it.value().toString();

				it = object.constFind("refresh_token");
				if (it != object.constEnd()) m_refreshToken = it.value().toString();
#else
				m_expiresIn = sc.property("expires_in").toInt32();
				m_accessToken = sc.property("access_token").toString();
				m_refreshToken = sc.property("access_token").toString();
#endif
				m_lastAccessTokenTime = QDateTime::currentDateTime();

				emit accessTokenReceived(m_accessToken, m_refreshToken);

				if (m_login.indexOf('@') > -1)
				{
					// we have an email, we need to request the login
					requestUserInfo();
				}
				else if (m_damnToken.isEmpty())
				{
					// login is good, request DAmn token
					requestDAmnToken();
				}
				else if (!m_filesToUpload.isEmpty())
				{
					StashFile file = m_filesToUpload.front();

					requestStash(file.filename, file.room);
				}
			}
			else if (error == "invalid_grant" || error == "invalid_request")
			{
				// we are fixing the error ourself
				status.clear();

				// refresh token invalid
				m_refreshToken.clear();

				loginOAuth2();
			}
		}
		else if (path.endsWith("/submit"))
		{
			if (status == "success")
			{
				QString folder;
				qint64 stashId, folderId;

#ifdef USE_QT5
				// seconds number before a session expire
				it = object.constFind("stashid");
				if (it != object.constEnd()) stashId = (qint64)it.value().toDouble();

				it = object.constFind("folder");
				if (it != object.constEnd()) folder = it.value().toString();

				it = object.constFind("folderid");
				if (it != object.constEnd()) folderId = (qint64)it.value().toDouble();
#else
				stashId = (qint64)sc.property("stashid").toNumber();
				folder = sc.property("folder").toString();
				folderId = (qint64)sc.property("folderid").toNumber();
#endif

				StashFile file = m_filesToUpload.front();

				m_filesToUpload.pop_front();

				if (!m_filesToUpload.isEmpty())
				{
					StashFile newFile = m_filesToUpload.front();

					requestStash(newFile.filename, newFile.room);
				}

				emit imageUploaded(file.room, QString("http://sta.sh/0%1").arg(base36enc(stashId)));
			}
		}
		else
		{
			qDebug() << "JSON data for " << url << "not processed" << json;
		}
	
		if (status == "error")
		{
			emit errorReceived(tr("API error: %1").arg(errorDescription.isEmpty() ? error:errorDescription));
		}
	}
	else if (redirection.endsWith("/login?bad_form=1"))
	{
		emit errorReceived(tr("Bad form"));
	}
	else if (url.endsWith("/join/oauth2"))
	{
		// when using oauth2
		if (redirection.endsWith("/join/blank"))
		{
			// login successful, authorize the application now
			requestAuthorization();
		}
		else
		{
			// wrong login
			emit errorReceived(tr("Login '%1' doesn't exist").arg(m_login));
		}
	}
	else if (redirection.startsWith("kdamn://"))
	{
#ifdef USE_QT5
		QUrlQuery query(QUrl(redirection).query());

		if (query.hasQueryItem("code"))
		{
			requestToken(query.queryItemValue("code"));
		}
		else if (query.hasQueryItem("error"))
		{
			emit errorReceived(query.queryItemValue("error_description"));
		}
#else
		QRegExp reg("^kdamn://oauth2/login\\?code=([0-9]+)$");

		if (reg.indexIn(redirection) > -1)
		{
			requestToken(reg.cap(1));
		}
#endif
		else
		{
			emit errorReceived(tr("Unknown error while redirected to %1").arg(redirection));
		}
	}
	else if (url.endsWith("/users/login"))
	{
		if (redirection.endsWith(QString("/wrong-password")))
		{
			emit errorReceived(tr("Login '%1' doesn't exist").arg(m_login));
		}
		else if (redirection.endsWith(QString("/wrong-password?username=%1").arg(m_login)))
		{
			emit errorReceived(tr("Wrong password for user %1").arg(m_login));
		}
		else
		{
			requestAuthToken();
		}
	}
	else if (url.endsWith("/Botdom"))
	{
		QRegExp reg("dAmn_Login\\( \"([A-Za-z0-9_-]+)\", \"([a-f0-9]{32})\"");

		QString html = reply->readAll();

		if (reg.indexIn(html) > -1)
		{
			m_login = reg.cap(1);
			m_damnToken = reg.cap(2);

			emit damnTokenReceived(m_login, m_damnToken);
		}
		else
		{
			emit errorReceived(tr("Unable to find dAmn_Login"));
		}
	}
	else if (url.indexOf("/applications") > -1 && url.indexOf(QString("client_id=%1").arg(m_clientId)) > -1)
	{
		// TODO: ask authorization to user
		authorizeApplication(true);
	}
	else if (url.endsWith("/rockedout"))
	{
		QString validationToken, validationKey;

		QString html = reply->readAll();

		QRegExp reg;
		
		reg.setPattern("name=\"validate_token\" value=\"([0-9a-f]{20})\"");

		if (reg.indexIn(html) > -1)
		{
			validationToken = reg.cap(1);
		}

		reg.setPattern("name=\"validate_key\" value=\"([0-9]{10})\"");

		if (reg.indexIn(html) > -1)
		{
			validationKey = reg.cap(1);
		}

		if (!validationToken.isEmpty() && !validationKey.isEmpty())
		{
			loginSite(validationToken, validationKey);
		}
		else
		{
			emit errorReceived(tr("Unable to find validate token or key"));
		}
	}
	else if (redirection.endsWith("/join/oauth2"))
	{
		// when using oauth2 and wrong password
		emit errorReceived(tr("Wrong password for user %1").arg(m_login));
	}
	else if (!redirection.isEmpty() && redirection != url)
	{
		qDebug() << "Redirected from" << url << "to" << redirection;

		get(redirection, url);
	}
	else
	{
		QString html = reply->readAll();

		qDebug() << html;
	}
}

void OAuth2::onReplyError(QNetworkReply::NetworkError error)
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	QString url;

	if (reply) url = reply->url().toString();

	switch(error)
	{
		case QNetworkReply::AuthenticationRequiredError:
		emit errorReceived(tr("Authentication required while accessing %1").arg(url));
		break;

		default:
		qDebug() << "Error:" << error;
		emit errorReceived(tr("Network error: %1").arg(error));
		break;
	}
}

void OAuth2::onSslErrors(const QList<QSslError> &errors)
{
	qDebug() << "SSL Errors:" << errors;

	foreach(const QSslError &error, errors)
	{
		emit errorReceived(tr("SSL errors: %1").arg(error.errorString()));
	}
}

void OAuth2::onAuthentication(QNetworkReply *reply, QAuthenticator *auth)
{
	auth->setUser(QString::number(m_clientId));
	auth->setPassword(m_clientSecret);
}

void OAuth2::onProxyAuthentication(const QNetworkProxy &proxy, QAuthenticator *auth)
{
	qDebug() << "proxy auth";

	emit errorReceived(tr("Proxy authentication required"));
}
