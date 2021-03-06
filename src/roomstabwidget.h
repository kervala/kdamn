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

#ifndef ROOMSTABWIDGET_H
#define ROOMSTABWIDGET_H

#include "damn.h"

class RoomFrame;
class ServerFrame;
class NotesFrame;
class NoteFrame;
class HtmlFormatting;

class RoomsTabWidget : public QTabWidget
{
	Q_OBJECT

public:
	RoomsTabWidget(QWidget *parent);
	virtual ~RoomsTabWidget();

	bool createServerFrame();
	ServerFrame* getServerFrame();

	bool createNotesFrame();
	bool removeNotesFrame();
	NotesFrame* getNotesFrame();

	bool createNoteFrame();
	bool removeNoteFrame();
	NoteFrame* getNoteFrame();

	bool createRoomFrame(const QString &room);
	bool removeRoomFrame(const QString &room);
	RoomFrame* getRoomFrame(const QString &room);
	RoomFrame* getCurrentRoomFrame();

	void setServer(const QString &text);

	void login();
	void updateConfig();

public slots:
	// slots activated from DAmn signals
	void onRequestDAmnToken();
	void onText(const QString &room, const QString &user, EMessageType type, const QString &text, bool html);
	void onConnectServer();
	void onDisconnectServer(bool reconnect);
	void onReconnectServer();
	void onUsers(const QString &room, const QStringList &users);
	void onJoinRoom(const QString &room);
	void onPartRoom(const QString &room, const QString &reason);
	void onUserJoin(const QString &room, const QString &user, bool show);
	void onUserPart(const QString &room, const QString &user, const QString &reason, bool show);
	void onUserKick(const QString &room, const QString &user, const QString &by);
	void onUserPriv(const QString &room, const QString &user, const QString &by, const QString &pc);
	void onPrivClass(const QString &room, const QString &privclass, const QString &by, const QString &privs);
	void onError(const QString &error);
	void onAfk(bool enabled, const QString &message);

	// slots activated from OAuth2 signals
	void onLoggedIn();
	void onReceiveFolders();
	void onReceiveDAmnToken(const QString &login, const QString &authtoken);
	void onUploadImage(const QString &room, const QString &url);
	void onReceiveNotes(int count);
	void onUpdateNotes(const QString &folderId, int offset, int count);
	void onUpdateNote(const QString &folderId, const QString &noteId);
	void onPrepareNote();
	void onSendNote(const QString &noteId);

	// slots activated from widgets
	void onRoomFocus(int index);

	void checkMessages();

protected:
	void changeEvent(QEvent *e);

	void updateSystrayIcon(const QString &room, const QString &user, const QString &html);

	void updateScreenCss();
	void updateLogCss();

	void playSound(const QString &filename);

	QString getCssFromStyle(const QString &style) const;


	QTimer *m_messagesTimer;
	QTimer *m_reconnectTimer;

	HtmlFormatting *m_formatHtml;
	HtmlFormatting *m_formatText;

	QString m_screenCss;
	QString m_logCss;
};

#endif
