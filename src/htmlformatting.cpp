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

#include "common.h"
#include "htmlformatting.h"
#include "moc_htmlformatting.cpp"
#include "configfile.h"
#include "utils.h"
#include "damnuser.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

HtmlFormatting::HtmlFormatting(QObject *parent):QObject(parent)
{
}

HtmlFormatting::~HtmlFormatting()
{
}

QString HtmlFormatting::formatUserPriv(const QString &user, const QString &by, const QString &privclass, bool html) const
{
	// TODO: the whole line is bold
	return formatLineSystem(tr("** %1 has been made a member of %2 by %3 *").arg(formatUser(user, html)).arg(formatPrivClass(privclass, html)).arg(formatUser(by, html)), html);
}

QString HtmlFormatting::formatPrivClass(const QString &privclass, const QString &by, const QString &privs, bool html) const
{
	return formatLineSystem(tr("** Privilege class %1 has been updated by %2 with: %3").arg(formatPrivClass(privclass, html)).arg(formatUser(by, html)).arg(privs), html);
}

QString HtmlFormatting::formatUserJoin(const QString &user, bool html) const
{
	return formatLineSystem(tr("** %1 has joined").arg(formatUser(user, html)), html);
}

QString HtmlFormatting::formatUserPart(const QString &user, const QString &reason, bool html) const
{
	return formatLineSystem(tr("** %1 has left").arg(formatUser(user, html)) + (!reason.isEmpty() ? QString(" [%1]").arg(reason):""), html);
}

QString HtmlFormatting::formatUserKick(const QString &user, const QString &by, bool html) const
{
	return formatLineSystem(tr("** %1 has been kicked by %2").arg(formatUser(user, html)).arg(formatUser(by, html)), html);
}

QString HtmlFormatting::formatJoinRoom(const QString &room, bool html) const
{
	return formatLineNormal(tr("*** You have joined %1 *").arg(formatRoom(room, html)), html);
}

QString HtmlFormatting::formatPartRoom(const QString &room, const QString &reason, bool html) const
{
	return formatLineNormal(tr("*** You have left %1 *").arg(formatRoom(room, html)) + (!reason.isEmpty() ? QString(" [%1]").arg(reason):""), html);
}

QString HtmlFormatting::formatMessageText(const QString &user, const QString &text, bool html) const
{
	QString formattedText;

	if (html)
	{
		QString login = ConfigFile::getInstance()->getLogin().toLower();

		// don't alert if we talk to ourself
		if (login != user.toLower())
		{
			// check if username mentioned in HTML code
			if (searchUserInHtml(login, text))
			{
				// hightlight username in HTML code
				formattedText = highlightUserInHtml(login, text);
			}
			else
			{
				formattedText = text;
			}
		}
		else
		{
			formattedText = formatStyle(text, true, "myself");
		}
	}
	else
	{
		formattedText = text;
	}

	return formatLineNormal(formatUser(QString("<%1>").arg(user), html) + " " + formattedText, html);
}

QString HtmlFormatting::formatMessageAction(const QString &user, const QString &text, bool html) const
{
	QString formattedText;

	if (html)
	{
		QString login = ConfigFile::getInstance()->getLogin().toLower();

		// don't alert if we talk to ourself
		if (login != user.toLower())
		{
			// check if username mentioned in HTML code
			if (searchUserInHtml(login, text))
			{
				// hightlight username in HTML code
				formattedText = highlightUserInHtml(login, text);
			}
		}
		else
		{
			formattedText = formatStyle(text, true, "myself");
		}
	}
	else
	{
		formattedText = text;
	}

	return formatLineNormal(formatStyle(formatUser(QString("* %1").arg(user), html) + " " + formattedText, html, "emote"), html);
}

QString HtmlFormatting::formatMessageTopic(const QString &user, const QString &text, bool html) const
{
	QString formattedText;

	if (text.isEmpty())
	{
		formattedText = tr("** Topic removed by %1 *").arg(formatUser(user, html));
	}
	else
	{
		formattedText = tr("** Topic changed by %1: %2 *").arg(formatUser(user, html)).arg(text);
	}

	return formatLineSystem(formattedText, html);
}

QString HtmlFormatting::formatMessageTitle(const QString &user, const QString &text, bool html) const
{
	QString formattedText;

	if (text.isEmpty())
	{
		formattedText = tr("** Title removed by %1 *").arg(formatUser(user, html));
	}
	else
	{
		formattedText = tr("** Title changed by %1: %2 *").arg(formatUser(user, html)).arg(text);
	}

	return formatLineSystem(formattedText, html);
}

QString HtmlFormatting::formatMessageTopicFirst(const QString &user, const QString &text, bool html) const
{
	if (text.isEmpty()) return QString();

	return formatLineNormal(text, html);
}

QString HtmlFormatting::formatMessageTitleFirst(const QString &user, const QString &text, bool html) const
{
	return QString();
}

QString HtmlFormatting::formatStyle(const QString &text, bool html, const QString &style) const
{
	if (html) return QString("<span class=\"%1\">%2</span>").arg(style).arg(text);

	return text;
}

QString HtmlFormatting::formatUser(const QString &user, bool html) const
{
	return formatStyle(html ? encodeEntities(user, "<>"):user, html, "username");
}

QString HtmlFormatting::formatRoom(const QString &room, bool html) const
{
	return formatStyle("#" + room, html, "room");
}

QString HtmlFormatting::formatPrivClass(const QString &privclass, bool html) const
{
	return formatStyle(privclass, html, "privclass");
}

QString HtmlFormatting::formatTimestamp(bool html) const
{
	// check if user doesn't want to display timestamp
	if (!ConfigFile::getInstance()->getDisplayTimestamps()) return QString();

	QString timestamp = QTime::currentTime().toString();

	return QString(html ? "<span class=\"timestamp\">%1</span> ":"[%1] ").arg(timestamp);
}

QString HtmlFormatting::formatLine(const QString &text, bool html, const QString &style) const
{
	if (html) return QString("<div class=\"%1\">%2%3</div>").arg(style).arg(formatTimestamp(true)).arg(text);

	return formatTimestamp(false) + text;
}

QString HtmlFormatting::formatLineNormal(const QString &text, bool html) const
{
	return formatLine(text, html, "normal");
}

QString HtmlFormatting::formatLineSystem(const QString &text, bool html) const
{
	return formatLine(text, html, "system");
}

QString HtmlFormatting::formatLineError(const QString &text, bool html) const
{
	return formatLine(text, html, "error");
}

bool HtmlFormatting::searchUserInHtml(const QString &user, const QString &html) const
{
	// lower HTML code
	QString lowerTextWithoutHtml = html.toLower();

	// remove HTML code
	lowerTextWithoutHtml.remove(QRegExp("<[^>]+>"));

	// found at least one occurence of login
	return lowerTextWithoutHtml.indexOf(user.toLower()) > -1;
}

bool HtmlFormatting::searchUserInText(const QString &user, const QString &text) const
{
	// found at least one occurence of login
	return text.toLower().indexOf(user.toLower()) > -1;
}

QString HtmlFormatting::highlightUserInHtml(const QString &user, const QString &html) const
{
	QString login = user;

	// use original username case
	DAmnUser *damnUser = DAmn::getInstance()->getUser(user);
	if (damnUser) login = damnUser->getName();

	// format it in HTML
	QString formattedLogin = formatStyle(login, true, "highlight");
	QString lowerUser = user.toLower();

	QString finalHtml;
	QString lowerHtml = html.toLower();

	QRegExp htmlReg("<[^>]+>");

	int begin = 0, end = 0;

	while(begin > -1)
	{
		end = htmlReg.indexIn(lowerHtml, begin);

		QString textPart, htmlPart;

		if (end > -1)
		{
			// found HTML code after text
			// take text from begin to end-begin
			textPart = html.mid(begin, end-begin);
			htmlPart = html.mid(end, end+htmlReg.matchedLength());

			begin = end+1;
		}
		else
		{
			// no HTML code after text
			// take text from begin to the end
			textPart = html.mid(begin);

			begin = end;
		}

		QString textPartlower = textPart.toLower();

		int pos = 0;

		// search all users in part
		while((pos = textPartlower.indexOf(lowerUser, pos)) > -1)
		{
			// replace login by highlighted HTML code
			textPart.replace(pos, user.length(), formattedLogin);

			pos += user.length();
		}

		// append this text
		finalHtml += textPart + htmlPart;
	}

	return finalHtml;
}
