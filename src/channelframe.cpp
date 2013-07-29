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
#include "configfile.h"
#include "damn.h"
#include "systrayicon.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

ChannelFrame::ChannelFrame(QWidget *parent, const QString &channel):QFrame(parent), m_channel(channel), m_focus(false)
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

void ChannelFrame::setAction(const QString &user, const QString &text)
{
	outputBrowser->append(QString("<div class=\"normal\">%1<span class=\"username\">%2</span> %3</div>").arg(getTimestamp()).arg(user).arg(text));

	startAnimations(text);
	updateSystrayIcon(user, text);
}

void ChannelFrame::setText(const QString &user, const QString &text)
{
	outputBrowser->append(QString("<div class=\"normal\">%1<span class=\"username\">&lt;%2&gt;</span> %3</div>").arg(getTimestamp()).arg(user).arg(text));

	startAnimations(text);
	updateSystrayIcon(user, text);
}

void ChannelFrame::setSystem(const QString &text)
{
	outputBrowser->append(QString("<div class=\"system\">%1%2</div>").arg(getTimestamp()).arg(text));

	startAnimations(text);
}

void ChannelFrame::setUsers(const QStringList &users)
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

void ChannelFrame::userJoin(const QString &user)
{
	setSystem(tr("%1 has joined").arg(user));

	QStringList users = m_usersModel->stringList();

	if (users.indexOf(user) == -1)
	{
		users << user;

		m_usersModel->setStringList(users);
	}
}

void ChannelFrame::userPart(const QString &user, const QString &reason)
{
	setSystem(tr("%1 has left").arg(user) + (!reason.isEmpty() ? QString(" [%1]").arg(reason):""));

	QStringList users = m_usersModel->stringList();

	users.removeAll(user);

	m_usersModel->setStringList(users);
}

void ChannelFrame::onSend()
{
	QStringList lines = inputEdit->getLines();

	if (DAmn::getInstance()->send(m_channel, lines))
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

void ChannelFrame::startAnimations(const QString &html)
{
	// parse HTML code to find local images
	QRegExp reg("\"(file://([^\"]+))\"");

	int pos = 0;

	while((pos = reg.indexIn(html, pos)) != -1)
	{
		// found an URL
		QString url = reg.cap(1);

		QHash<QMovie*, QUrl>::iterator it = m_urls.begin();

		bool found = false;

		while(it != m_urls.end())
		{
			if (it->toString() == url)
			{
				found = true;
				break;
			}

			++it;
		}

		if (!found) new AnimationStart(url, this);

		pos += reg.matchedLength();
	}
}

void ChannelFrame::setFocus(bool focus)
{
	if (focus == m_focus) return;

	m_focus = focus;

	QHash<QMovie*, QUrl>::iterator it = m_urls.begin();

	while(it != m_urls.end())
	{
		if (m_focus)
		{
			it.key()->start();
		}
		else
		{
			it.key()->stop();
		}

		++it;
	}

	if (m_focus)
	{
		SystrayIcon::getInstance()->setStatus(m_channel, StatusNormal);
		inputEdit->setFocus();
	}
}

void ChannelFrame::updateSystrayIcon(const QString &user, const QString &html)
{
	if (m_focus) return;

	QString login = ConfigFile::getInstance()->getLogin().toLower();

	// don't alert if we talk to ourself
	if (login == user.toLower()) return;

	SystrayStatus oldStatus = SystrayIcon::getInstance()->getStatus(m_channel);
	SystrayStatus newStatus = html.toLower().indexOf(login) > -1 ? StatusTalkMe:StatusTalkOther;

	if (newStatus > oldStatus) SystrayIcon::getInstance()->setStatus(m_channel, newStatus);
}

bool ChannelFrame::addAnimation(const QString& file)
{
	QUrl url(file);

	QMovie* movie = new QMovie(this);

	movie->setFileName(url.toLocalFile());

	if (!movie->isValid())
	{
		delete movie;
		new AnimationStart(file, this);

		return false;
	}

	QString test = movie->format();

	if (movie->frameCount() < 2) return false;
	
	m_urls.insert(movie, url);

	connect(movie, SIGNAL(frameChanged(int)), this, SLOT(animate(int)));

	// start only when in foreground
	if (m_focus) movie->start();

	return true;
}

void ChannelFrame::animate(int frame)
{
	if (QMovie* movie = qobject_cast<QMovie*>(sender()))
	{
		outputBrowser->document()->addResource(QTextDocument::ImageResource, m_urls.value(movie), movie->currentPixmap());
		outputBrowser->setLineWrapColumnOrWidth(outputBrowser->lineWrapColumnOrWidth()); // causes reload
	}
}
