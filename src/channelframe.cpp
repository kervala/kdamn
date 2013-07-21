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
#include "channelframe.h"
#include "damn.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

ChannelFrame::ChannelFrame(QWidget *parent, const QString &channel):QFrame(parent), m_channel(channel)
{
	setupUi(this);

	connect(inputEdit, SIGNAL(returnPressed()), this, SLOT(onSend()));

	connect(outputBrowser, SIGNAL(anchorClicked(QUrl)), this, SLOT(onUrl(QUrl)));

	m_usersModel = new QStringListModel(this);
	usersView->setModel(m_usersModel);

	outputBrowser->document()->setDefaultStyleSheet(".timestamp { color: #999; }\n.username { font-weight: bold; }\n.error { color:  #f00; }");
}

ChannelFrame::~ChannelFrame()
{
}

void ChannelFrame::setText(const QString &user, const QString &text)
{
	outputBrowser->append(QString("<div class=\"normal\">%1<span class=\"username\">&lt;%2&gt;</span> %3</div>").arg(getTimestamp()).arg(user).arg(text));
}

void ChannelFrame::setTopic(const QString &topic)
{
	outputBrowser->append(tr("<div class=\"title\">Topic is %1</div>").arg(topic));
}

void ChannelFrame::setTitle(const QString &title)
{
	outputBrowser->append(tr("<div class=\"title\">Title is %1</div>").arg(title));
}

void ChannelFrame::setUsers(const QList<DAmnMember> &users)
{
	QStringList list;
	int min = 65536;
	int max = 0;

	foreach(const DAmnMember &member, users)
	{
//		int width = usersView->fontMetrics().boundingRect(member.name).width();
		int width = usersView->fontMetrics().width(member.name)+10;

		if (width < min) min = width;
		if (width > max) max = width;

		list << member.name;
	}

	m_usersModel->setStringList(list);

	usersView->setMinimumWidth(min);
	usersView->setMaximumWidth(max);
}

void ChannelFrame::onSend()
{
	if (DAmn::getInstance()->send(m_channel, inputEdit->text()))
	{
		inputEdit->validate();
	}
}

void ChannelFrame::onUrl(const QUrl &url)
{
	QDesktopServices::openUrl(url);
}

QString ChannelFrame::getTimestamp() const
{
	QString timestamp = QTime::currentTime().toString();

	return QString("<span class=\"timestamp\">%1</span> ").arg(timestamp);
}

