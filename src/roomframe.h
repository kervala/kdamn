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

#ifndef ROOMFRAME_H
#define ROOMFRAME_H

#include "ui_roomframe.h"

#include "tabframe.h"

struct DAmnMember;

class RoomFrame : public TabFrame, public Ui::RoomFrame
{
	Q_OBJECT

public:
	RoomFrame(QWidget *parent, const QString &room);
	virtual ~RoomFrame();

	void setAction(const QString &user, const QString &text, bool html);
	void setText(const QString &user, const QString &text, bool html);
	void setSystem(const QString &text, bool html);
	void setTopic(const QString &user, const QString &topic, bool html);
	void setTitle(const QString &user, const QString &title, bool html);

	void setSystem(const QString &text);

	void setUsers(const QStringList &users);
	void userJoin(const QString &user);
	void userPart(const QString &user, const QString &reason);

	QString getRoom() const { return m_room; }

	bool setFocus(bool focus);

public slots:
	// when user press enter
	void onSend();

	// when user double-click on a user
	void onUserDoubleClicked(const QModelIndex &index);

	void onHomepageUser();
	void onPromoteUser();
	void onDemoteUser();
	void onBanUser();
	void onUnbanUser();
	void onKickUser();

protected:
	void updateSplitter();

	QStringListModel *m_usersModel;
	QString m_room;
};

#endif
