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
#include "damn.h"
#include "oembed.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

OAuth2* OAuth2::s_instance = NULL;

OAuth2::OAuth2(QObject *parent):QObject(parent), m_manager(NULL), m_clientId(0), m_expiresIn(0), m_inboxId(0), m_noteId(0), m_logged(false), m_reconnectAfterLogout(false)
{
	if (s_instance == NULL) s_instance = this;

	m_clientId = 474;
	m_clientSecret = "6a8b3dacb0d41c5d177d6f189df772d1";

	new OEmbed(this);
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

void OAuth2::clear()
{
	// parameters
	m_login.clear();
	m_password.clear();
	m_accessToken.clear();
	m_refreshToken.clear();
	m_damnToken.clear();
	m_filesToUpload.clear();

	// session
	m_sessionId.clear();
	m_validateToken.clear();
	m_validateKey.clear();
}

void OAuth2::addUserAgent(QNetworkRequest &req) const
{
	QString userAgent = GetUserAgent();

	if (userAgent.isEmpty()) return;

#ifdef USE_QT5
	req.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
#else
	req.setRawHeader("User-Agent", userAgent.toLatin1());
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

	return true;
}

bool OAuth2::getFilename(const QString &url, const QString &filename)
{
	init();

	QNetworkRequest req;
	req.setUrl(QUrl(url));

	addUserAgent(req);

	QNetworkReply *reply = m_manager->get(req);
	reply->setUserData(0, new StringUserData(filename));

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

	return true;
}

Folder OAuth2::getFolder(const QString &id) const
{
	return m_folders.value(id);
}

void OAuth2::emitImageDownloaded(const QString &md5, bool downloaded)
{
	emit imageDownloaded(md5, downloaded);
}

void OAuth2::emitErrorReceived(const QString &error)
{
	emit errorReceived(error);
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
	params.addQueryItem("response_type", "code");
	params.addQueryItem("redirect_uri", REDIRECT_APP);
	params.addQueryItem("scope", "basic");
	params.addQueryItem("state", "");
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

	return post(OAUTH2LOGIN_URL, data);
}

bool OAuth2::logout(bool reconnect)
{
	if (!m_logged)
	{
		clear();

		emit loggedOut(reconnect);

		return false;
	}

	if (m_sessionId.isEmpty() || m_validateToken.isEmpty() || m_validateKey.isEmpty())
	{
		m_reconnectAfterLogout = reconnect;

		m_actions.push_front(ActionLogout);

		return get(SESSIONS_URL);
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

	return post(LOGOUT_URL, data, SESSIONS_URL);
}

bool OAuth2::prepareNote()
{
	return get(NOTES_URL);
}

bool OAuth2::checkUrlChanges(const QString &url)
{
	return getFilename(url, url);
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

bool OAuth2::processImage(const QString &url, const QByteArray &content)
{
	DAmnImage *image = NULL;

	bool downloaded = false;
	bool error = true;
	QString md5;

	if (DAmn::getInstance()->getWaitingImageFromRemoteUrl(url, image))
	{
		// MD5 should be correct if image was found
		md5 = image->md5;

		if (md5.isEmpty())
		{
			qDebug() << "Error: strange case where MD5 is empty";
		}
		else if (!content.isEmpty())
		{
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

				downloaded = true;
				error = false;
			}
		}
		else
		{
			// only retry 10 times
			if (image->retries < 10)
			{
				error = false;

				// retry to download it after a delay of 1 second
				if (!DAmn::getInstance()->downloadImage(*image, 1000))
				{
					if (image->downloaded)
					{
						// already downloaded
						downloaded = true;
					}
					else
					{
						qDebug() << "Error: strange case where downloadImage returned false and image not already downloaded";
					}
				}
				else
				{
					// image will be downloaded again
				}
			}
		}
	}

	if (downloaded)
	{
		emit imageDownloaded(md5, true);
	}

	// image not found after 5 retries or unknown error
	if (error)
	{
		// remove message from waiting list
		if (md5.isEmpty()) md5 = QCryptographicHash::hash(url.toLatin1(), QCryptographicHash::Md5).toHex();

		emit imageDownloaded(md5, false);

		return false;
	}

	return true;
}

void OAuth2::onReply(QNetworkReply *reply)
{
	QString redirection = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl().toString();
	QString url = reply->url().toString();
	int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	QByteArray content = reply->readAll();
	QNetworkReply::NetworkError errorCode = reply->error();
	QString errorString = reply->errorString();
	QString filename = reply->userData(0) ? ((StringUserData*)reply->userData(0))->text:"";

	// always delete QNetworkReply to avoid memory leaks
	reply->deleteLater();
	reply = NULL;

#ifdef _DEBUG
	qDebug() << "URL:" << url;
	if (!redirection.isEmpty()) qDebug() << "Redirection:" << redirection;
	if (errorCode != QNetworkReply::NoError) qDebug() << "HTTP" << statusCode << "error" << errorCode << errorString;
#endif

	// if status code 302 (redirection), 404 (not found) or 401 (unauthorized), we can clear content
	if (statusCode == 302 || statusCode == 404 || statusCode == 401) content.clear();

	if (content.isEmpty())
	{
		if (!redirection.isEmpty())
		{
			// redirection and no content
			processRedirection(redirection, url);
		}
		else if (errorCode != QNetworkReply::NoError)
		{
			bool displayError = true;

			if (statusCode == 404)
			{
				if (url.indexOf(QRegExp("\\." + GetSupportedImageFormatsFilter() + "(\\?([0-9]+))?$")) > -1)
				{
					displayError = !processImage(url, content);
				}
			}

			if (displayError)
			{
				emit errorReceived(tr("Network error: %1 (HTTP %2)").arg(errorString).arg(statusCode));

				emit imageDownloaded(filename, false);
			}
		}
		else
		{
		}
	}
	else
	{
		if (!redirection.isEmpty())
		{
			DAmnImage *image = NULL;

			if (DAmn::getInstance()->getWaitingImageFromRemoteUrl(url, image))
			{
				// change remote URL to new redirection
				image->remoteUrl = redirection;

				// redirection and content
				processRedirection(redirection, url);
			}
			else
			{
				emit errorReceived(tr("Error: both content (%1) and redirection (%2) are defined").arg(url).arg(redirection));
			}
		}
		else
		{
			if (url == filename)
			{
				processUrlChanges(content, url);
			}
			else
			{
				// content and no redirection (ignore errors because content can be parsed)
				processContent(content, url, filename);
			}
		}
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
			logout(m_reconnectAfterLogout);
			break;

			case ActionRequestDAmnToken:
			requestDAmnToken();
			break;

			case ActionRequestPlacebo:
			requestPlacebo();
			break;

			default:
			break;
		}
	}
}

bool OAuth2::processNextUpload()
{
	if (m_filesToUpload.isEmpty()) return false;

	int index = -1;

	for(int i = 0; i < m_filesToUpload.size(); ++i)
	{
		if (m_filesToUpload[i].status == StashFile::StatusUploading)
		{
			// a file is currently uploading, we'll upload it later
			return false;
		}
	}

	for(int i = 0; i < m_filesToUpload.size(); ++i)
	{
		if (m_filesToUpload[i].status == StashFile::StatusNone)
		{
			index = i;
			break;
		}
	}

	// all files are currently uploading or no file to upload
	if (index < 0) return false;

	StashFile &file = m_filesToUpload[index];
	file.status = StashFile::StatusUploading;

	if (requestStash(file.filename, file.room)) return true;

	// file not found, so remove it
	m_filesToUpload.removeAt(index);

	// process next file
	return processNextUpload();
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

void OAuth2::processContent(const QByteArray &content, const QString &url, const QString &filename)
{
	QUrl urlTemp(url);

#ifdef USE_QT5
	QUrlQuery query(urlTemp);
#else
	QUrl query = urlTemp;
#endif

	bool isJson = content.startsWith("{");
	bool isHtml = content.startsWith("<");

	if (!isHtml && !isJson /* url.indexOf(QRegExp("\\." + OAuth2::getSupportedImageFormatsFilter() + "(\\?([0-9]+))?$")) > -1 */)
	{
		processImage(url, content);
	}
	else if (isJson)
	{
		if (url.startsWith(DIFI_URL))
		{
			// received DiFi content
			processDiFi(content);
		}
		else if (url.contains("oembed"))
		{
			// received oEmbed content
			OEmbed::getInstance()->processContent(content, url, filename);
		}
		else
		{
			// received DA API content
			processJson(content, urlTemp.path(), filename);
		}
	}
	else if (url.startsWith(OAUTH2LOGIN_URL))
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
	else if (m_logged)
	{
		parseSessionVariables(content);

		processNextAction();
	}
	else
	{
		emit errorReceived(tr("Something goes wrong... URL: %1").arg(url));

		qDebug() << content;
	}
}

void OAuth2::processUrlChanges(const QByteArray &content, const QString &url)
{
	QString html = QString::fromUtf8(content);

	// remove minor changes from HTML
	html.remove(QRegExp("dwaitdatex(_|-)[0-9a-f]+"));
	html.remove(QRegExp("549m[0-9a-f]+"));
	html.remove(QRegExp("\"script\", [0-9]+\\);"));
	html.remove(QRegExp("<span title=\"[^\"]*\">"));

	QString md5 = QCryptographicHash::hash(html.toUtf8(), QCryptographicHash::Md5).toHex();

	emit urlChecked(url, md5);
}

void OAuth2::processRedirection(const QString &redirection, const QString &url)
{
	if (url.startsWith(AUTHORIZE_URL) && redirection.startsWith(JOIN_URL))
	{
		// fill OAuth2 login form
		login();
	}
	else if (url.startsWith(OAUTH2LOGIN_URL) && redirection.startsWith(AUTHORIZE_URL))
	{
		m_logged = true;

		emit loggedIn();

		requestAuthorization();
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
	else if (url == LOGOUT_URL)
	{
		m_logged = false;

		logout(m_reconnectAfterLogout);
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

			// TODO: remove #new-note from id
			emit noteSent(id);
		}
		else
		{
			emit errorReceived(tr("Error when sending note: %1").arg(redirection));
		}
	}
	else if (url.startsWith(SESSIONS_URL) && redirection.contains(SESSIONS_URL))
	{
		// session has been closed somewhere else
		m_logged = false;

		// reconnect automatically
		logout(true);
	}
	else if (redirection != url)
	{
		redirect(redirection, url);
	}
	else
	{
		emit errorReceived(tr("Something goes wrong... URL: %1 Redirection: %2").arg(url).arg(redirection));
	}
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
