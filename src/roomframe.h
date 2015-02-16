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

	void setUsers(const QStringList &users);
	void userJoin(const QString &user);
	void userPart(const QString &user);

	QString getRoom() const { return m_room; }

	bool setFocus(bool focus);

	void setReceiveHtml(bool on);
	void setReceiveText(bool on);

	void appendHtml(const QString &html);
	void appendText(const QString &text);

	void closeHtmlLog();
	void closeTextLog();

	void updateCssScreen(const QString &css);
	void updateCssFile(const QString &css);

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

	void onKeyPressed(QKeyEvent *e);

protected:
	void updateSplitter();

	QStringListModel *m_usersModel;
	QString m_room;

	bool m_textReceived;
	bool m_htmlReceived;

	LogFile m_htmlFile;
	LogFile m_textFile;
};

#endif
