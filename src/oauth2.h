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

#ifndef OAUTH2_H
#define OAUTH2_H

class OAuth2 : public QObject
{
	Q_OBJECT

public:
	OAuth2(QObject *parent);
	virtual ~OAuth2();

	static OAuth2* getInstance() { return s_instance; }

	bool login(const QString &login, const QString &password);
	bool loginSite(const QString &login, const QString &password);

	bool requestAuthorization();
	bool requestAuthToken();
	bool requestToken(const QString &code);
	bool requestUserInfo();
	bool requestDAmnToken();
	bool authorizeApplication(bool authorize);
	bool get(const QString &url, const QString &referer = "");
	bool uploadToStash(const QString &filename);

	static QString getSupportedImageFormatsFilter();

signals:
	void errorReceived(const QString &error);
	void damnTokenReceived(const QString &login, const QString &damntoken);
	void imageDownloaded(const QString &md5);

public slots:
	void onReply(QNetworkReply *reply);
	void onAuthentication(const QNetworkProxy &proxy, QAuthenticator *auth);
	void onReplyError(QNetworkReply::NetworkError error);
	void onSslErrors(const QList<QSslError> &errors);

private:
	QNetworkAccessManager *m_manager;
	QString m_token;
	QString m_refreshToken;
	QString m_login;
	QString m_damnToken;
	int m_clientId;
	QString m_clientSecret;
	int m_expiresIn;

	static OAuth2 *s_instance;
};

#endif
