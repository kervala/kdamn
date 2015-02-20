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

#ifndef HTMLFORMATTING_H
#define HTMLFORMATTING_H

#include "damn.h"

class RoomFrame;
class ServerFrame;
class NotesFrame;
class NoteFrame;

class HtmlFormatting : public QObject
{
	Q_OBJECT

public:
	HtmlFormatting(bool html, QObject *parent);
	virtual ~HtmlFormatting();

	QString formatUserPriv(const QString &user, const QString &by, const QString &privclass) const;
	QString formatPrivClass(const QString &privclass, const QString &by, const QString &privs) const;
	QString formatUserJoin(const QString &user) const;
	QString formatUserPart(const QString &user, const QString &reason) const;
	QString formatUserKick(const QString &user, const QString &by) const;
	QString formatJoinRoom(const QString &room) const;
	QString formatPartRoom(const QString &room, const QString &reason) const;

	QString formatMessageText(const QString &user, const QString &text) const;
	QString formatMessageAction(const QString &user, const QString &text) const;
	QString formatMessageTopic(const QString &user, const QString &text) const;
	QString formatMessageTitle(const QString &user, const QString &text) const;
	QString formatMessageTopicFirst(const QString &user, const QString &text) const;
	QString formatMessageTitleFirst(const QString &user, const QString &text) const;

	QString formatStyle(const QString &text, const QString &style) const;
	QString formatUser(const QString &user) const;
	QString formatRoom(const QString &room) const;
	QString formatPrivClass(const QString &privclass) const;
	QString formatTimestamp(bool html) const;

	QString formatLine(const QString &text, const QString &style) const;
	QString formatLineNormal(const QString &text) const;
	QString formatLineSystem(const QString &text) const;
	QString formatLineError(const QString &text) const;
	QString formatLineTitle(const QString &text) const;
	QString formatLineTopic(const QString &text) const;

	bool searchUser(const QString &user, const QString &text) const;

	QString highlightUser(const QString &user, const QString &html) const;

private:
	bool m_html;
};

#endif
