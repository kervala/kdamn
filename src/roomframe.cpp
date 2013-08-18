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

#include "common.h"
#include "roomframe.h"
#include "moc_roomframe.cpp"
#include "configfile.h"
#include "damn.h"
#include "systrayicon.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

RoomFrame::RoomFrame(QWidget *parent, const QString &room):TabFrame(parent), m_room(room)
{
	setupUi(this);

	connect(inputEdit, SIGNAL(returnPressed()), this, SLOT(onSend()));

	m_usersModel = new QStringListModel(this);
	usersView->setModel(m_usersModel);

	outputBrowser->setRoom(room);
}

RoomFrame::~RoomFrame()
{
}

void RoomFrame::setAction(const QString &user, const QString &text, bool html)
{
	if (html) outputBrowser->setAction(user, text, html);
}

void RoomFrame::setText(const QString &user, const QString &text, bool html)
{
	outputBrowser->setText(user, text, html);
}

void RoomFrame::setSystem(const QString &text, bool html)
{
	outputBrowser->setSystem(text, html);
}

void RoomFrame::setTopic(const QString &user, const QString &topic, bool html)
{
	if (topic.isEmpty())
	{
		outputBrowser->setSystem(tr("Topic was removed by %1").arg(user), html);
	}
	else
	{
		outputBrowser->setSystem(tr("Topic changed by %1: %2").arg(user).arg(topic), html);
	}
}

void RoomFrame::setTitle(const QString &user, const QString &title, bool html)
{
	if (title.isEmpty())
	{
		outputBrowser->setSystem(tr("Title was removed by %1").arg(user), html);
	}
	else
	{
		outputBrowser->setSystem(tr("Title changed by %1: %2").arg(user).arg(title), html);
	}
}

void RoomFrame::setSystem(const QString &text)
{
	outputBrowser->setSystem(text, true);
	outputBrowser->setSystem(text, false);
}

void RoomFrame::setUsers(const QStringList &users)
{
	int min = 65536;
	int max = 0;

	foreach(const QString &user, users)
	{
//		int width = usersView->fontMetrics().boundingRect(member.name).width();
		int width = usersView->fontMetrics().width(user)+10;

		if (width < min) min = width;
		if (width > max) max = width;
	}

	m_usersModel->setStringList(users);

	usersView->setMinimumWidth(min);
	usersView->setMaximumWidth(max);

	inputEdit->setUsers(users);
}

void RoomFrame::userJoin(const QString &user)
{
	setSystem(tr("%1 has joined").arg(user));

	QStringList users = m_usersModel->stringList();

	if (users.indexOf(user) == -1)
	{
		users << user;

		m_usersModel->setStringList(users);
	}
}

void RoomFrame::userPart(const QString &user, const QString &reason)
{
	setSystem(tr("%1 has left").arg(user) + (!reason.isEmpty() ? QString(" [%1]").arg(reason):""));

	QStringList users = m_usersModel->stringList();

	users.removeAll(user);

	m_usersModel->setStringList(users);
}

void RoomFrame::onSend()
{
	QStringList lines = inputEdit->getLines();

	if (DAmn::getInstance()->send(m_room, lines))
	{
		inputEdit->validate();
	}
}

bool RoomFrame::setFocus(bool focus)
{
	if (!TabFrame::setFocus(focus)) return false;

	outputBrowser->setFocus(focus);

	if (m_focus) inputEdit->setFocus();

	return true;
}
