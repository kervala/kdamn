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
	HtmlFormatting(QObject *parent);
	virtual ~HtmlFormatting();

	QString formatUserPriv(const QString &user, const QString &by, const QString &privclass, bool html) const;
	QString formatPrivClass(const QString &privclass, const QString &by, const QString &privs, bool html) const;
	QString formatUserJoin(const QString &user, bool html) const;
	QString formatUserPart(const QString &user, const QString &reason, bool html) const;
	QString formatUserKick(const QString &user, const QString &by, bool html) const;
	QString formatJoinRoom(const QString &room, bool html) const;
	QString formatPartRoom(const QString &room, const QString &reason, bool html) const;

	QString formatMessageText(const QString &user, const QString &text, bool html) const;
	QString formatMessageAction(const QString &user, const QString &text, bool html) const;
	QString formatMessageTopic(const QString &user, const QString &text, bool html) const;
	QString formatMessageTitle(const QString &user, const QString &text, bool html) const;
	QString formatMessageTopicFirst(const QString &user, const QString &text, bool html) const;
	QString formatMessageTitleFirst(const QString &user, const QString &text, bool html) const;

	QString formatStyle(const QString &text, bool html, const QString &style) const;
	QString formatUser(const QString &user, bool html) const;
	QString formatRoom(const QString &room, bool html) const;
	QString formatPrivClass(const QString &privclass, bool html) const;
	QString formatTimestamp(bool html) const;

	QString formatLine(const QString &text, bool html, const QString &style) const;
	QString formatLineNormal(const QString &text, bool html) const;
	QString formatLineSystem(const QString &text, bool html) const;
	QString formatLineError(const QString &text, bool html) const;

	bool searchUserInHtml(const QString &user, const QString &html) const;
	bool searchUserInText(const QString &user, const QString &text) const;

	QString highlightUserInHtml(const QString &user, const QString &html) const;
};

#endif
