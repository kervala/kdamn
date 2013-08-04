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

#ifndef ROOMSTABWIDGET_H
#define ROOMSTABWIDGET_H

#include "damn.h"

class RoomFrame;
class ServerFrame;

class RoomsTabWidget : public QTabWidget
{
	Q_OBJECT

public:
	RoomsTabWidget(QWidget *parent);
	virtual ~RoomsTabWidget();

	bool createServerFrame();
	ServerFrame* getServerFrame();

	bool createRoomFrame(const QString &room);
	bool removeRoomFrame(const QString &room);
	RoomFrame* getRoomFrame(const QString &room);
	RoomFrame* getCurrentRoomFrame();

	void setSystem(const QString &text);

	void login();

public slots:
	// slots activated from DAmn signals
	void onRequestDAmnToken();
	void onText(const QString &room, const QString &user, MessageType type, const QString &text, bool html);
	void onReceiveAuthtoken(const QString &login, const QString &authtoken);
	void onConnectServer();
	void onUsers(const QString &room, const QStringList &users);
	void onJoinRoom(const QString &room);
	void onPartRoom(const QString &room, const QString &reason);
	void onUserJoin(const QString &room, const QString &user, bool show);
	void onUserPart(const QString &room, const QString &user, const QString &reason, bool show);
	void onUserPriv(const QString &room, const QString &user, const QString &by, const QString &pc);
	void onError(const QString &error);

	// slots activated from widgets
	void onRoomFocus(int index);

protected:
	void updateSystrayIcon(const QString &room, const QString &user, const QString &html);
};

#endif