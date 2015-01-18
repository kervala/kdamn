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

#include "folder.h"

// deviantART URLs
#define BASE_URL "www.deviantart.com"
#define HTTPS_URL "https://"BASE_URL
#define HTTP_URL "http://"BASE_URL
#define LOGIN_URL HTTPS_URL"/users/login"
#define OAUTH2LOGIN_URL HTTPS_URL"/join/oauth2login"
#define LOGOUT_URL HTTPS_URL"/settings/force-logout"
#define ROCKEDOUT_URL HTTPS_URL"/users/rockedout"
#define NOTES_URL HTTPS_URL"/messages/notes/#1_0"
#define SENDNOTE_URL HTTP_URL"/messages/notes/send"
#define OAUTH2_URL HTTPS_URL"/api/v1/oauth2"
#define OAUTH2_TOKEN_URL HTTPS_URL"/oauth2/token"
#define DIFI_URL HTTPS_URL"/global/difi.php"
#define REDIRECT_APP "kdamn://oauth2/login"
#define UPDATE_URL "http://kervala.net/utils/update.php"
#define AUTHORIZE_URL HTTPS_URL"/oauth2/authorize"
#define JOIN_URL HTTPS_URL"/join/oauth2"
#define SESSIONS_URL HTTPS_URL"/settings/sessions"
#define OEMBED_URL "http://backend.deviantart.com/oembed"

struct NoteForm
{
	QString recipientsHash;
	QString subjectHash;
	QStringList hashes;
};

struct StashFile
{
	enum eStatus
	{
		StatusNone,
		StatusUploading,
		StatusUploaded
	};

	QString filename;
	QString room;
	eStatus status;

	StashFile():status(StatusNone)
	{
	}

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
		ActionCheckNotes,
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
	bool requestAuthorization(); // private
	bool uploadToStash(const QString &filename, const QString &room);
	bool requestImageInfo(const QString &url);
	bool requestDAmnToken();

	bool prepareNote();
	bool sendNote(const Note &note);

	// DiFi

	// MessageCenter
	bool requestMessageCenterGetFolders();
	bool requestMessageCenterGetViews();

	// Notes
	bool requestNotesCreateFolder(const QString &name, const QString &parentFolderId = QLatin1String("0"));
	bool requestNotesDelete(const QStringList &notesIds);
	bool requestNotesDeleteNote(const QString &noteId);
	bool requestNotesDeleteFolder(const QString &folderId);
	bool requestNotesDisplay(const QString &noteId);
	bool requestNotesDisplayDraft(const QString &noteId);
	bool requestNotesDisplayFolder(const QString &folderId, int offset);
	bool requestNotesDisplayNote(const QString &folderId, const QString &noteId);
	bool requestNotesMarkAsRead(const QStringList &notesIds);
	bool requestNotesMarkAsUnread(const QStringList &notesIds);
	bool requestNotesMove(const QStringList &notesIds, const QString &folderId);
	bool requestNotesMoveNote(const QString &folder, const QString &noteId);
	bool requestNotesPlaceboCall();
	bool requestNotesPreview(const QString &text, bool includeSignature);
	bool requestNotesRenameFolder(const QString &folderId, const QString &folderName);
	bool requestNotesSaveDraft(const QString &recipients, const QString &subject, const QString &body);
	bool requestNotesStar(const QStringList &notesIds);
	bool requestNotesUnstar(const QStringList &notesIds);

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
	void imageUploaded(const QString &room, const QString &stashUrl);
	void notesUpdated(const QString &folderId, int offset, int count);
	void noteUpdated(const QString &folderId, const QString &noteId);
	void notesReceived(int count);
	void newVersionDetected(const QString &url, const QString &date, uint size, const QString &version);
	void uploadProgress(qint64 readBytes, qint64 totalBytes);
	void noteSent(const QString &id);
	void notePrepared();

	// DiFi signals
	void foldersReceived();
	void notesFolderCreated(const QString &name, const QString &id);
	void notesDeleted();
	void notesFolderDeleted(const QString &id);
	void notesFolderRenamed(const QString &folderId, const QString &folderName);
	void notesRead(const QStringList &noteIds);
	void notesUnread(const QStringList &noteIds);
	void notesDraftSaved(const QString &draftId);
	void notesStarred(const QStringList &noteIds);
	void notesUnstarred(const QStringList &noteIds);
	void notesPreviewReceived(const QString &html);
	void notesMoved();

public slots:
	void onReply(QNetworkReply *reply);
	void onAuthentication(QNetworkReply *reply, QAuthenticator *auth);
	void onProxyAuthentication(const QNetworkProxy &proxy, QAuthenticator *auth);
	void onUploadFinished();

private:
	// OAuth2 steps
	bool requestAccessToken(const QString &code = "");
	bool requestPlacebo();
	bool requestUserInfo();
	bool requestStash(const QString &filename, const QString &room);
	bool authorizeApplication(const QString &validateKey, const QString &validateToken, bool authorize);

	// site process
	QString getAuthorizationUrl() const;

	void addUserAgent(QNetworkRequest &req) const;

	bool requestGet(const QString &cls, const QString &method, const QString &args);
	bool requestPost(const QString &cls, const QString &method, const QString &args);

	bool requestNotes(const QString &method, const QString &args);
	bool requestMessageCenter(const QString &method, const QString &args);

	void redirect(const QString &url, const QString &referer);
	void processContent(const QByteArray &content, const QString &url);
	void processRedirection(const QString &redirection, const QString &url);
	void processDiFi(const QByteArray &content);
	void processJson(const QByteArray &content, const QString &path, const QString &url);
	void processNewVersions(const QByteArray &content);
	void processNextAction();
	bool parseSessionVariables(const QByteArray &content);
	bool parseNotesFolders(const QByteArray &content);
	bool parseNotesForm(const QByteArray &content);
	bool parseFolder(const QString &html, Folder &folder, int &count);
	bool parseNote(const QString &html, Note &note);

	bool parseNotesIdsAndCount(const QVariantMap &response, QStringList &notesIds, int &count);

	// DiFi parsing methods
	bool parseMessageCenterGetFolders(const QVariantMap &response);
	bool parseMessageCenterGetViews(const QVariantMap &response);

	bool parseNotesCreateFolder(const QVariantMap &response);
	bool parseNotesDelete(const QVariantMap &response);
	bool parseNotesDeleteFolder(const QVariantMap &response);
	bool parseNotesDisplayFolder(const QVariantMap &response);
	bool parseNotesDisplayNote(const QVariantMap &response);
	bool parseNotesMarkAsRead(const QVariantMap &response);
	bool parseNotesMarkAsUnread(const QVariantMap &response);
	bool parseNotesMove(const QVariantMap &response);
	bool parseNotesPreview(const QVariantMap &response);
	bool parseNotesRenameFolder(const QVariantMap &response);
	bool parseNotesSaveDraft(const QVariantMap &response);
	bool parseNotesStar(const QVariantMap &response);
	bool parseNotesUnstar(const QVariantMap &response);


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

	Folders m_folders;

	NoteForm m_noteForm;

	static QString s_userAgent;
	static OAuth2 *s_instance;
};

#endif
