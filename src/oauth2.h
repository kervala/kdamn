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

#ifndef OAUTH2_H
#define OAUTH2_H

#include "notesmodel.h"

// deviantART URLs
#define BASE_URL "www.deviantart.com"
#define HTTPS_URL "https://"BASE_URL
#define HTTP_URL "http://"BASE_URL
#define LOGIN_URL HTTPS_URL"/users/login"
#define LOGOUT_URL HTTPS_URL"/settings/force-logout"
#define ROCKEDOUT_URL HTTPS_URL"/users/rockedout"
#define OAUTH2_URL HTTPS_URL"/api/v1/oauth2"
#define OAUTH2_TOKEN_URL HTTPS_URL"/oauth2/token"
#define CHAT_URL "http://chat.deviantart.com/chat/Botdom"
#define DIFI_URL HTTPS_URL"/global/difi.php"
#define REDIRECT_APP "kdamn://oauth2/login"
#define UPDATE_URL "http://kervala.net/utils/update.php"

struct StashFile
{
	QString filename;
	QString room;

	bool operator == (const StashFile &other)
	{
		return other.filename.toLower() == filename.toLower();
	}
};

typedef QList<StashFile> StashFiles;
typedef StashFiles::iterator StashFilesIterator;

class OAuth2 : public QObject
{
	Q_OBJECT

public:
	OAuth2(QObject *parent);
	virtual ~OAuth2();

	enum eOAuth2Action
	{
		ActionLoginOAuth2,
		ActionLoginWeb,
		ActionLogout,
		ActionCheckFolders,
		ActionCheckNotes,
		ActionDisplayFolder,
		ActionDisplayNote,
		ActionRequestAccessToken,
		ActionRequestDAmnToken,
		ActionRequestAuthorization,
		ActionUploadStash,
		ActionRequestPlacebo
	};

	static OAuth2* getInstance() { return s_instance; }

	void init();
	void setLogin(const QString &login) { m_login = login; }
	void setPassword(const QString &password) { m_password = password; }
	void setDAmnToken(const QString &token) { m_damnToken = token; }
	void setAccessToken(const QString &access, const QString &refresh) { m_accessToken = access; m_refreshToken = refresh; }

	bool login();
	bool logout();
	bool uploadToStash(const QString &filename, const QString &room);
	bool requestImageInfo(const QString &url, const QString &room);
	bool requestDAmnToken();

	// DiFi
	bool requestMessageFolders();
	bool requestMessageViews();
	bool requestDisplayFolder(const QString &folderId, int offset);
	bool requestDisplayNote(const QString &folderId, int noteId);
	
	static QString getSupportedImageFormatsFilter();
	static QString getUserAgent();

	bool checkUpdates();

	bool get(const QString &url, const QString &referer = "");
	bool post(const QString &url, const QByteArray &data = QByteArray(), const QString &referer = "");

	bool isLogged() const { return m_logged; }

	Folder getFolder(const QString &id) const;

signals:
	void loggedIn();
	void loggedOut();
	void errorReceived(const QString &error);
	void accessTokenReceived(const QString &access, const QString &refresh);
	void damnTokenReceived(const QString &login, const QString &damntoken);
	void imageDownloaded(const QString &md5);
	void imageUploaded(const QString &room, const QString &stashId);
	void folderReceived(const QString &id);
	void notesReceived(int count);
	void newVersionDetected(const QString &url, const QString &date, uint size, const QString &version);
	void uploadProgress(qint64 readBytes, qint64 totalBytes);

public slots:
	void onReply(QNetworkReply *reply);
	void onAuthentication(QNetworkReply *reply, QAuthenticator *auth);
	void onProxyAuthentication(const QNetworkProxy &proxy, QAuthenticator *auth);
	void onReplyError(QNetworkReply::NetworkError error);
	void onSslErrors(const QList<QSslError> &errors);
	void onUploadFinished();

private:
	// OAuth2 steps
	bool requestAuthorization();
	bool requestAccessToken(const QString &code = "");
	bool requestPlacebo();
	bool requestUserInfo();
	bool requestStash(const QString &filename, const QString &room);
	bool authorizeApplication(const QString &validateKey, const QString &validateToken, bool authorize);

	// site process
	QString getAuthorizationUrl() const;
	bool loginSite(const QString &validationToken, const QString &validationKey);
	bool requestAuthToken();

	void addUserAgent(QNetworkRequest &req) const;

	void redirect(const QString &url, const QString &referer);
	void processDiFi(const QByteArray &content);
	void processJson(const QByteArray &content, const QString &path);
	void processNewVersions(const QByteArray &content);
	void processNextAction();
	void parseSessionVariables(const QByteArray &content);
	bool parseFolder(const QString &html, Folder &folder);
	bool parseNote(const QString &html, Note &note);

	QNetworkAccessManager *m_manager;
	QString m_login;
	QString m_password;
	QString m_accessToken;
	QString m_refreshToken;
	QString m_damnToken;
	int m_clientId;
	QString m_clientSecret;
	int m_expiresIn;
	QDateTime m_lastAccessTokenTime;
	StashFiles m_filesToUpload;
	int m_inboxId;
	int m_noteId;
	bool m_logged;

	// session variables
	QString m_sessionId;
	QString m_validateToken;
	QString m_validateKey;

	QList<eOAuth2Action> m_actions;

	QMap<QString, Folder> m_folders;

	static QString s_userAgent;
	static OAuth2 *s_instance;
};

#endif
