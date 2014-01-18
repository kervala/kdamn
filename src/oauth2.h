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

	static OAuth2* getInstance() { return s_instance; }

	void init();
	void setLogin(const QString &login) { m_login = login; }
	void setPassword(const QString &password) { m_password = password; }
	void setDAmnToken(const QString &token) { m_damnToken = token; }
	void setAccessToken(const QString &access, const QString &refresh) { m_accessToken = access; m_refreshToken = refresh; }

	bool login(bool oauth2);
	bool uploadToStash(const QString &filename, const QString &room);

	static QString getSupportedImageFormatsFilter();
	static QString getUserAgent();

	bool get(const QString &url, const QString &referer = "");
	bool post(const QString &url, const QByteArray &data, const QString &referer = "");

signals:
	void errorReceived(const QString &error);
	void accessTokenReceived(const QString &access, const QString &refresh);
	void damnTokenReceived(const QString &login, const QString &damntoken);
	void imageDownloaded(const QString &md5);
	void imageUploaded(const QString &room, const QString &url);

public slots:
	void onReply(QNetworkReply *reply);
	void onAuthentication(QNetworkReply *reply, QAuthenticator *auth);
	void onProxyAuthentication(const QNetworkProxy &proxy, QAuthenticator *auth);
	void onReplyError(QNetworkReply::NetworkError error);
	void onSslErrors(const QList<QSslError> &errors);
	void onUploadProgress(qint64 readBytes, qint64 totalBytes);
	void onUploadFinished();

private:
	// OAuth2 steps
	bool loginOAuth2();
	bool requestAuthorization();
	bool requestToken(const QString &code = "");
	bool requestPlacebo();
	bool requestUserInfo();
	bool requestDAmnToken();
	bool requestStash(const QString &filename, const QString &room);
	bool authorizeApplication(bool authorize);

	// site process
	bool getValidateToken();
	bool loginSite(const QString &validationToken, const QString &validationKey);
	bool requestAuthToken();

	void addUserAgent(QNetworkRequest &req) const;

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

	static QString s_userAgent;
	static OAuth2 *s_instance;
};

#endif
