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

QString OAuth2::s_userAgent;
OAuth2* OAuth2::s_instance = NULL;

OAuth2::OAuth2(QObject *parent):QObject(parent), m_manager(NULL), m_clientId(0), m_expiresIn(0), m_inboxId(0), m_noteId(0), m_logged(false)
{
	if (s_instance == NULL) s_instance = this;

	m_clientId = 474;
	m_clientSecret = "6a8b3dacb0d41c5d177d6f189df772d1";
}

OAuth2::~OAuth2()
{
	s_instance = NULL;
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

Folder OAuth2::getFolder(const QString &id) const
{
	return m_folders.value(id);
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

bool OAuth2::prepareNote()
{
	return get(NOTES_URL);
}

bool OAuth2::sendNote(const Note &note)
{
	if (m_noteForm.hashes.isEmpty()) return false;

#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("validate_token", m_validateToken);
	params.addQueryItem("validate_key", m_validateKey);
	params.addQueryItem("recipients", "");
	params.addQueryItem("ref", "new-note");
	params.addQueryItem("parentid", "");

	// only one hash must be filled
	foreach(const QString &hash, m_noteForm.hashes)
	{
		params.addQueryItem(hash, hash == m_noteForm.recipientsHash ? note.recipients.join(", "):"");
	}

	params.addQueryItem("friends", "");
	params.addQueryItem(m_noteForm.subjectHash, note.subject);

	params.addQueryItem("body", note.text);

	if (note.hasSignature) params.addQueryItem("signature", "on");

	QByteArray data;

#ifdef USE_QT5
	data = params.query().toUtf8();
#else
	data = params.encodedQuery();
#endif

	return post(QString("%1?%2").arg(SENDNOTE_URL).arg(m_validateKey), data, NOTES_URL);
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
		s_userAgent = QString("%1/%2 (%3)").arg(QApplication::applicationName()).arg(QApplication::applicationVersion()).arg(system);
	}

	return s_userAgent;
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

	qDebug() << "URL:" << url;
	qDebug() << "Redirection:" << redirection;

#ifdef USE_QT5
	QUrlQuery query(reply->url().query());
#else
	QUrl query = reply->url();
#endif

	bool isJson = content.indexOf("{") == 0 || query.hasQueryItem("access_token") || query.hasQueryItem("grant_type") || query.hasQueryItem("url");
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
		if (url.startsWith(DIFI_URL))
		{
			processDiFi(content);
		}
		else if (url.startsWith(UPDATE_URL))
		{
			processNewVersions(content);
		}
		else
		{
			processJson(content, path);
		}
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
		QRegExp reg(QString("^%1\\?code=([0-9a-f]+)$").arg(REDIRECT_APP));

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

			requestMessageCenterGetViews();

			emit loggedIn();
		}
		else
		{
			requestAuthToken();
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
	else if (url == NOTES_URL)
	{
		// prepare note
		parseSessionVariables(content);
		parseNotesFolders(content);
		parseNotesForm(content);

		emit notePrepared();
	}
	else if (url == LOGOUT_URL)
	{
		m_logged = false;

		m_sessionId.clear();
		m_validateToken.clear();
		m_validateKey.clear();

		emit loggedOut();
	}
	else if (url.startsWith(SENDNOTE_URL))
	{
		int pos = redirection.indexOf("?success=");

		if (pos > -1)
		{
			// mail successfully sent

			// reset note form
			m_noteForm = NoteForm();

			QString id = redirection.mid(pos + 9);

			emit noteSent(id);
		}
		else
		{
		}
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

		case QNetworkReply::RemoteHostClosedError:
		// silently ignore
		break;

		default:
		emit errorReceived(tr("Network error: %1").arg(error));
		break;
	}
}

void OAuth2::onSslErrors(const QList<QSslError> &errors)
{
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
	emit errorReceived(tr("Proxy authentication required"));
}

void OAuth2::redirect(const QString &url, const QString &referer)
{
	qDebug() << "Redirected from" << referer << "to" << url;

	get(url, referer);
}

void OAuth2::processNextAction()
{
	if (!m_actions.isEmpty())
	{
		eOAuth2Action action = m_actions.front();

		m_actions.pop_front();

		switch(action)
		{
			case ActionCheckNotes:
			requestMessageCenterGetViews();
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

			default:
			break;
		}
	}
}

bool OAuth2::parseSessionVariables(const QByteArray &content)
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

	if (!m_validateKey.isEmpty() && !m_validateToken.isEmpty()) return true;

	emit errorReceived(tr("Unable to find validate token or key"));

	return false;
}

bool OAuth2::parseNotesFolders(const QByteArray &content)
{
	QRegExp reg;

	reg.setPattern("data-folderid=\"([a-z0-9]+)\" href=\"#([a-z0-9_]+)\" rel=\"([0-9]+)\" title=\"([^\"]+)\"");

	int pos = 0;

	while((pos = reg.indexIn(content, pos)) > -1)
	{
		QString folderId = reg.cap(1);
		QString folderName = reg.cap(4);

		// create folder
		Folder folder;
		folder.id = folderId;
		folder.name = folderName;

		folder.maxOffset = 0;
		folder.count = 0;

		m_folders[folderId] = folder;

		pos += reg.matchedLength();
	}

	return true;
}

bool OAuth2::parseNotesForm(const QByteArray &content)
{
	// clear previous hashes
	m_noteForm.hashes.clear();
	m_noteForm.recipientsHash.clear();
	m_noteForm.subjectHash.clear();

	QRegExp reg;

	// parse all hashes from CSS (the one not hidden is recipients)
	reg.setPattern("#l_([0-9a-f]{20})\\{(display:none)?\\}");

	int pos = 0;
	int lastPos = pos;

	while((pos = reg.indexIn(content, pos)) > -1)
	{
		QString hash = reg.cap(1);
		bool hidden = !reg.cap(2).isEmpty();

		m_noteForm.hashes << hash;

		if (!hidden)
		{
			m_noteForm.recipientsHash = hash;
		}

		pos += reg.matchedLength();
		lastPos = pos;
	}

	pos = lastPos;

	// parse subject hash
	reg.setPattern("for=\"([a-f0-9]+)\" >Subject</label>");

	pos = reg.indexIn(content, pos);

	if (pos < 0) return false;

	m_noteForm.subjectHash = reg.cap(1);

	return true;
}
