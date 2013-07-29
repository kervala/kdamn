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

#ifndef CHANNELFRAME_H
#define CHANNELFRAME_H

#include "ui_channelframe.h"

#include <QFrame>

struct DAmnMember;

class ChannelFrame : public QFrame, public Ui::ChannelFrame
{
	Q_OBJECT

public:
	ChannelFrame(QWidget *parent, const QString &channel);
	virtual ~ChannelFrame();

	void setAction(const QString &user, const QString &text);
	void setText(const QString &user, const QString &text);
	void setSystem(const QString &text);

	void setUsers(const QStringList &users);
	void userJoin(const QString &user);
	void userPart(const QString &user, const QString &reason);

	QString getChannel() const { return m_channel; }

	void startAnimations(const QString &html);
	bool addAnimation(const QString &url);
	void setFocus(bool focus);

	void updateSystrayIcon(const QString &user, const QString &html);

public slots:
	// when user press enter
	void onSend();

	// when user click on a link
	void onUrl(const QUrl &url);

	void animate(int frame);

protected:
	QString getTimestamp() const;

	QStringListModel *m_usersModel;
	QString m_channel;
	QHash<QMovie*, QUrl> m_urls;
	bool m_focus;
};

class AnimationStart : public QObject
{
	Q_OBJECT

public:
	AnimationStart(const QString &url, ChannelFrame *frame):QObject(frame), m_url(url), m_frame(frame)
	{
		QTimer::singleShot(1000, this, SLOT(timeout()));
	}

public slots:
	void timeout()
	{
		m_frame->addAnimation(m_url);
	}

private:
	ChannelFrame *m_frame;
	QString m_url;
};

#endif
