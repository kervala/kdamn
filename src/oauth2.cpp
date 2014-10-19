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

#ifdef USE_QT5
#define GET_JSON(x) object[x]
#define GET_JSON_STRING(x) object[x].toString()
#define GET_JSON_DOUBLE(x) object[x].toDouble()
#else
#define GET_JSON(x) object.property(x)
#define GET_JSON_STRING(x) object.property(x).toString()
#define GET_JSON_DOUBLE(x) object.property(x).toNumber()
#endif

// DiFi : http://botdom.com/documentation/DiFi

// deviantART URLs
#define BASE_URL "www.deviantart.com"
#define HTTPS_URL "https://"BASE_URL
#define HTTP_URL "http://"BASE_URL
#define LOGIN_URL HTTPS_URL"/users/login"
#define LOGOUT_URL HTTPS_URL"/settings/force-logout"
#define ROCKEDOUT_URL HTTPS_URL"/users/rockedout"
#define DIFI_URL HTTPS_URL"/global/difi.php"
#define OAUTH2_URL HTTPS_URL"/api/v1/oauth2"
#define CHAT_URL "http://chat.deviantart.com/chat/Botdom"
#define REDIRECT_APP "kdamn://oauth2/login"

QString OAuth2::s_userAgent;
OAuth2* OAuth2::s_instance = NULL;

OAuth2::OAuth2(QObject *parent):QObject(parent), m_manager(NULL), m_clientId(0), m_expiresIn(0), m_inboxId(0), m_logged(false)
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

bool OAuth2::authorizeApplication(const QString &validateKey, const QString &validateToken, bool authorize)
{
#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("client_id", QString::number(m_clientId));
	params.addQueryItem("response_type", "code");
	params.addQueryItem("redirect_uri", REDIRECT_APP);
	params.addQueryItem("scope", "basic");
	params.addQueryItem("state", "");
	params.addQueryItem("authorized", authorize ? "1":"");
	params.addQueryItem("terms_agree[]", "1");
	params.addQueryItem("terms_agree[]", "0");
	params.addQueryItem("validate_key", validateKey);

	if (!validateToken.isEmpty())
	{
		params.addQueryItem("validate_token", validateToken);
	}

	QByteArray data;

#ifdef USE_QT5
	data = params.query().toUtf8();
#else
	data = params.encodedQuery();
#endif

	return post(QString("%1/settings/authorize_app").arg(HTTPS_URL), data);
}

bool OAuth2::login()
{
#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("client_id", QString::number(m_clientId));
	params.addQueryItem("subdomain", "www");
	params.addQueryItem("referrer", getAuthorizationUrl());
	params.addQueryItem("oauth2", "1");
	params.addQueryItem("username", m_login);
	params.addQueryItem("password", m_password);

	QByteArray data;

#ifdef USE_QT5
	data = params.query().toUtf8();
#else
	data = params.encodedQuery();
#endif

	return post(QString("%1/join/oauth2login").arg(HTTPS_URL), data);
}

bool OAuth2::logout()
{
	if (m_sessionId.isEmpty() || m_validateToken.isEmpty() || m_validateKey.isEmpty())
	{
		m_actions.push_front(ActionLogout);

		return get("https://www.deviantart.com/settings/sessions");
	}

#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("sessionid", m_sessionId);
	params.addQueryItem("validate_token", m_validateToken);
	params.addQueryItem("validate_key", m_validateKey);

	QByteArray data;

#ifdef USE_QT5
	data = params.query().toUtf8();
#else
	data = params.encodedQuery();
#endif

	return post(LOGOUT_URL, data, "https://www.deviantart.com/settings/sessions");
}

bool OAuth2::uploadToStash(const QString &filename, const QString &room)
{
	StashFile file;
	file.filename = filename;
	file.room = room;

	if (m_filesToUpload.indexOf(file) < 0) m_filesToUpload.push_back(file);

	m_actions.push_front(ActionUploadStash);

	// only check access token for the first file
	return m_filesToUpload.size() > 0 && requestPlacebo();
}

bool OAuth2::requestImageInfo(const QString &url, const QString &room)
{
	return get(QString("http://backend.deviantart.com/oembed?url=%1").arg(url));
}

QString OAuth2::getAuthorizationUrl() const
{
	return QString("%1/oauth2/authorize?response_type=code&client_id=%2&redirect_uri=%3").arg(HTTPS_URL).arg(m_clientId).arg(REDIRECT_APP);
}

bool OAuth2::requestAuthorization()
{
	if (!m_logged)
	{
		m_actions.push_front(ActionRequestAuthorization);

		return login();
	}

	return get(getAuthorizationUrl());
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

	return get(QString("%1/oauth2/token?client_id=%2&client_secret=%3&grant_type=%4").arg(HTTPS_URL).arg(m_clientId).arg(m_clientSecret).arg(query));
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

	QString mime;
	QString ext = info.suffix().toLower();

	if (ext == "png") mime = "image/png";
	else if (ext == "jpg" || ext == "jpeg") mime = "image/jpeg";
	else if (ext == "gif") mime = "image/gif";
	else mime = "application/binary";

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

	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onReplyError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));
	connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this, SLOT(onUploadProgress(qint64, qint64)));
	connect(reply, SIGNAL(finished()), this, SLOT(onUploadFinished()));

	BeginSystemProgress();

	return true;
}

bool OAuth2::requestMessageFolders()
{
	if (!m_logged)
	{
		m_actions.push_front(ActionCheckFolders);

		return login();
	}

	return get(QString("%1?c[]=MessageCenter;get_folders;&t=json").arg(DIFI_URL));
}

bool OAuth2::requestMessageViews()
{
	if (!m_inboxId)
	{
		m_actions.push_front(ActionCheckNotes);

		return requestMessageFolders();
	}

	return get(QString("%1?c[]=MessageCenter;get_views;%2,oq:notes_unread:0:0:f&t=json").arg(DIFI_URL).arg(m_inboxId));
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

			case QSysInfo::WV_4_0: system += "NT 4.0"; break; // Windows NT 4
			case QSysInfo::WV_5_0: system += "NT 5.0"; break; // Windows 2000
			case QSysInfo::WV_5_1: system += "NT 5.1"; break; // Windows XP
			case QSysInfo::WV_5_2: system += "NT 5.2"; break; // Windows Vista
			case QSysInfo::WV_6_0: system += "NT 6.0"; break; // Windows 7
			case QSysInfo::WV_6_1: system += "NT 6.1"; break; // Windows 8
			case QSysInfo::WV_6_2: system += "NT 6.2"; break; // Windows 8.1
			case QSysInfo::WV_6_3: system += "NT 6.3"; break;
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
		//s_userAgent = QString("%1/%2 (%3)").arg(QApplication::applicationName()).arg(QApplication::applicationVersion()).arg(system);
		s_userAgent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:24.0) Gecko/20100101 Firefox/24.0";
	}

	return s_userAgent;
}

bool OAuth2::loginSite(const QString &validationToken, const QString &validationKey)
{
#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("ref", "https://www.deviantart.com/users/loggedin");
	params.addQueryItem("username", m_login);
	params.addQueryItem("password", m_password);
	params.addQueryItem("remember_me", "1");
	params.addQueryItem("validate_token", validationToken);
	params.addQueryItem("validate_key", validationKey);

	QByteArray data;

#ifdef USE_QT5
	data = params.query().toUtf8();
#else
	data = params.encodedQuery();
#endif

	return post(LOGIN_URL, data, ROCKEDOUT_URL);
}

bool OAuth2::requestNotes()
{
#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("c[]", "\"Notes\",\"display_folder\",[\"unread\",0,false]");
	params.addQueryItem("t", "json");
	params.addQueryItem("ui", qobject_cast<Cookies*>(m_manager->cookieJar())->get("userinfo"));

	QByteArray data;

#ifdef USE_QT5
	data = params.query().toUtf8();
#else
	data = params.encodedQuery();
#endif

	return post(DIFI_URL, data, HTTPS_URL);
}

bool OAuth2::requestAuthToken()
{
	return get(CHAT_URL);
}

void OAuth2::onReply(QNetworkReply *reply)
{
	if (reply->error() == QNetworkReply::NoError)
	{
	}

	qobject_cast<Cookies*>(m_manager->cookieJar())->saveToDisk();

	QString redirection = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl().toString();
	QString url = reply->url().toString();
	QByteArray content = reply->readAll();

#ifdef _DEBUG
	qDebug() << "URL:" << url;
	qDebug() << "Redirection:" << redirection;
#endif

#ifdef USE_QT5
	QUrlQuery query(reply->url().query());
#else
	QUrl query = reply->url();
#endif

	bool isJson = query.hasQueryItem("access_token") || query.hasQueryItem("grant_type") || query.hasQueryItem("url");
	bool isDiFi = query.hasQueryItem("c[]") && query.hasQueryItem("t");
	QString path = reply->url().path();

	reply->deleteLater();
	reply = NULL;

	if (url.indexOf(QRegExp("\\." + OAuth2::getSupportedImageFormatsFilter() + "(\\?([0-9]+))?$")) > -1)
	{
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
		processJson(content, path);
	}
	else if (isDiFi)
	{
		processDiFi(content);
	}
	else if (url.endsWith("/join/oauth2login"))
	{
		// when using oauth2
		if (redirection.endsWith("/join/blank"))
		{
			m_logged = true;

			emit loggedIn();

			processNextAction();
		}
		else
		{
			if (content.indexOf("The username or password you entered was incorrect.") > -1)
			{
				// wrong login or password
				emit errorReceived(tr("Login '%1' or password is incorrect").arg(m_login));
			}
			else
			{
				// <span class="field_error" rel="password">
				emit errorReceived(tr("Unknown error for login '%1'").arg(m_login));
			}
		}
	}
	else if (url.endsWith("/join/oauth2"))
	{
		// fill OAuth2 login form
		login();
	}
	else if (redirection.startsWith(REDIRECT_APP))
	{
#ifdef USE_QT5
		QUrlQuery query(QUrl(redirection).query());

		if (query.hasQueryItem("code"))
		{
			requestAccessToken(query.queryItemValue("code"));
		}
		else if (query.hasQueryItem("error"))
		{
			emit errorReceived(query.queryItemValue("error_description"));
		}
#else
		QRegExp reg(QString("^%1\\?code=([0-9]+)$").arg(REDIRECT_APP));

		if (reg.indexIn(redirection) > -1)
		{
			requestAccessToken(reg.cap(1));
		}
#endif
		else
		{
			emit errorReceived(tr("Unknown error while redirected to %1").arg(redirection));
		}
	}
	else if (url.startsWith(LOGIN_URL))
	{
		if (redirection.endsWith("?bad_form=1"))
		{
			emit errorReceived(tr("Bad form"));
		}
		else if (redirection.endsWith(QString("/wrong-password")))
		{
			emit errorReceived(tr("Login '%1' doesn't exist").arg(m_login));
		}
		else if (redirection.endsWith(QString("/wrong-password?username=%1").arg(m_login)))
		{
			emit errorReceived(tr("Wrong password for user %1").arg(m_login));
		}
		else if (redirection.endsWith(".com"))
		{
			// redirection to user home page
			m_logged = true;

			requestMessageViews();

			emit loggedIn();
		}
		else
		{
			requestAuthToken();
		}
	}
	else if (url.endsWith("/Botdom"))
	{
		QRegExp reg("dAmn_Login\\( \"([A-Za-z0-9_-]+)\", \"([a-f0-9]{32})\"");

		if (reg.indexIn(content) > -1)
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
		QRegExp reg("name=\"validate_key\" value=\"([0-9]+)\"");

		if (reg.indexIn(content) > -1)
		{
			QString validateKey = reg.cap(1);

			// TODO: ask authorization to user
			authorizeApplication(validateKey, "", true);
		}
		else
		{
			emit errorReceived(tr("Unable to find validate_key"));
		}
	}
	else if (url.indexOf("/authorize_app") > -1)
	{
		QRegExp reg("name=\"validate_key\" value=\"([0-9]+)\"");

		if (reg.indexIn(content) > -1)
		{
			QString validateKey = reg.cap(1);

			reg.setPattern("name=\"validate_token\" value=\"([0-9a-f]+)\"");

			if (reg.indexIn(content) > -1)
			{
				QString validateToken = reg.cap(1);

				// TODO: ask authorization to user
				authorizeApplication(validateKey, validateToken, true);
			}
			else
			{
				emit errorReceived(tr("Unable to find validate_token"));
			}
		}
		else
		{
			emit errorReceived(tr("Unable to find validate_key"));
		}
	}
	else if (url == LOGOUT_URL/* && redirection.isEmpty() && !content.isEmpty() */)
	{
		m_logged = false;

		m_sessionId.clear();
		m_validateToken.clear();
		m_validateKey.clear();

		emit loggedOut();
	}
	else if (m_logged && redirection.isEmpty() && !content.isEmpty())
	{
		parseSessionVariables(content);

		processNextAction();
	}
	else if (!redirection.isEmpty() && redirection != url)
	{
		redirect(redirection, url);
	}
	else
	{
		emit errorReceived(tr("Something goes wrong... URL: %1").arg(url));

		qDebug() << content;
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

void OAuth2::redirect(const QString &url, const QString &referer)
{
	qDebug() << "Redirected from" << referer << "to" << url;

	get(url, referer);
}

void OAuth2::processDiFi(const QByteArray &content)
{
	// we received JSON response
	QString status, error, errorDescription;

#ifdef USE_QT5
	QJsonParseError jsonError;
	QJsonDocument doc = QJsonDocument::fromJson(content, &jsonError);

	if (jsonError.error != QJsonParseError::NoError)
	{
		emit errorReceived(jsonError.errorString());
		return;
	}

	QJsonObject object = doc.object();

	QJsonObject difi = object["DiFi"].toObject();

	if (!difi.isEmpty())
	{
		QJsonObject response = difi["response"].toObject();

		QJsonArray calls = response["calls"].toArray();

		foreach (const QJsonValue &itemValue, calls)
		{
			QJsonObject item = itemValue.toObject();

			QJsonObject request = item["request"].toObject();
			QString method = request["method"].toString();

			QJsonObject response = item["response"].toObject();

			status = response["status"].toString();

			if (status == "SUCCESS")
			{
				if (method == "get_folders")
				{
					QJsonArray content = response["content"].toArray();

					foreach(const QJsonValue &folderValue, content)
					{
						QJsonObject folder = folderValue.toObject();

						QString folderId = folder["folderid"].toString();
						QString title = folder["title"].toString();
						bool isInbox = folder["is_inbox"].toBool();

						qDebug() << folderId << title << isInbox;

						if (isInbox)
						{
							m_inboxId = folderId.toInt();
						}
					}
				}
				else if (method == "get_views")
				{
					QJsonArray views = response["content"].toArray();

					foreach(const QJsonValue &viewValue, views)
					{
						QJsonObject view = viewValue.toObject();

						QString offset = view["offset"].toString();
						QString length = view["length"].toString();
						int status = view["status"].toInt();

						QJsonObject result = view["result"].toObject();

						QString matches = result["matches"].toString();
						int count = result["count"].toInt();

						QJsonArray hits = result["hits"].toArray();

						emit notesReceived(matches.toInt());

						qDebug() << "notes" << matches;
					}
				}
			}
			else
			{
				QJsonObject content = response["content"].toObject();
				QJsonObject err = content["error"].toObject();

				QString errorCode = err["code"].toString();
				QString errorHuman = err["human"].toString();

				qDebug() << "error" << status << errorCode << errorHuman;
			}
		}
	}
	else
	{
		qDebug() << "JSON data not processed (not valid DiFi)" << content;
	}
#else
	QScriptEngine engine;
	QScriptValue object = engine.evaluate("(" + QString(content) + ")");
#endif

	// TODO: equivalent for Qt 4

	if (status == "error")
	{
		emit errorReceived(tr("DiFi error: %1").arg(errorDescription.isEmpty() ? error:errorDescription));
	}
	else
	{
		processNextAction();
	}
}

void OAuth2::processJson(const QByteArray &content, const QString &path)
{
	// we received JSON response
#ifdef USE_QT5
	QJsonParseError jsonError;
	QJsonDocument doc = QJsonDocument::fromJson(content, &jsonError);

	if (jsonError.error != QJsonParseError::NoError)
	{
		emit errorReceived(jsonError.errorString());
		return;
	}

	QJsonObject object = doc.object();
#else
	QScriptEngine engine;
	QScriptValue object = engine.evaluate("(" + QString(content) + ")");
#endif

	QString status = GET_JSON_STRING("status");
	QString error = GET_JSON_STRING("error");
	QString errorDescription = GET_JSON_STRING("error_description");

	if (path.endsWith("/whoami"))
	{
		m_login = GET_JSON_STRING("username");

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
			processNextAction();
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
			m_damnToken = GET_JSON_STRING("damntoken");

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
			m_expiresIn = (int)GET_JSON_DOUBLE("expires_in");
			m_accessToken = GET_JSON_STRING("access_token");
			m_refreshToken = GET_JSON_STRING("access_token");

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
	else if (path.endsWith("/submit"))
	{
		if (status == "success")
		{
			qint64 stashId = (qint64)GET_JSON_DOUBLE("stashid");
			QString folder = GET_JSON_STRING("folder");
			qint64 folderId = (qint64)GET_JSON_DOUBLE("folderid");

			if (!m_filesToUpload.isEmpty())
			{
				StashFile file = m_filesToUpload.front();

				m_filesToUpload.pop_front();

				emit imageUploaded(file.room, QString::number(stashId));
			}

			processNextAction();
		}
		else
		{
			qDebug() << "Error while uploading file";
		}
	}
	else if (path.endsWith("/oembed"))
	{
		QString title = GET_JSON_STRING("title");
		QString url = GET_JSON_STRING("url");
		QString author = GET_JSON_STRING("author_name");
		QString thumbnail = GET_JSON_STRING("thumbnail_url_150");
		int width = (int)GET_JSON_DOUBLE("width");
		int height = (int)GET_JSON_DOUBLE("height");
/*
		StashFile file = m_filesToUpload.front();

		m_filesToUpload.pop_front();

		processNextAction();

		emit imageInfo(file.room, QString::number(stashId));
*/
	}
	else
	{
		qDebug() << "JSON data for not processed" << content;
	}

	if (status == "error")
	{
		emit errorReceived(tr("API error: %1").arg(errorDescription.isEmpty() ? error:errorDescription));
	}
}

void OAuth2::processNextAction()
{
	if (!m_actions.isEmpty())
	{
		qDebug() << m_actions;

		eOAuth2Action action = m_actions.front();

		m_actions.pop_front();

		switch(action)
		{
			case ActionCheckFolders:
			requestMessageFolders();
			break;

			case ActionCheckNotes:
			requestMessageViews();
			break;

			case ActionRequestAuthorization:
			// login successful, authorize the application now
			requestAuthorization();
			break;

			case ActionLogout:
			logout();
			break;

			case ActionRequestDAmnToken:
			requestDAmnToken();
			break;

			case ActionUploadStash:

			if (!m_filesToUpload.isEmpty())
			{
				StashFile file = m_filesToUpload.front();

				requestStash(file.filename, file.room);
			}

			break;
		}
	}
	else
	{
		qDebug() << "Don't know what to do";
	}
}

void OAuth2::parseSessionVariables(const QByteArray &content)
{
	QRegExp reg;

	reg.setPattern("data-sessionid=\"([0-9a-f]{32})\"");

	if (reg.indexIn(content) > -1)
	{
		m_sessionId = reg.cap(1);
	}

	reg.setPattern("name=\"validate_token\" value=\"([0-9a-f]{20})\"");

	if (reg.indexIn(content) > -1)
	{
		m_validateToken = reg.cap(1);
	}

	reg.setPattern("name=\"validate_key\" value=\"([0-9]{10})\"");

	if (reg.indexIn(content) > -1)
	{
		m_validateKey = reg.cap(1);
	}

	if (m_sessionId.isEmpty())
	{
		emit errorReceived(tr("Unable to find validate token or key"));
	}
}
