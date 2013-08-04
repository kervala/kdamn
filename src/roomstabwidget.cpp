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
#include "roomstabwidget.h"
#include "moc_roomstabwidget.cpp"
#include "damn.h"
#include "oauth2.h"
#include "roomframe.h"
#include "serverframe.h"
#include "configfile.h"
#include "systrayicon.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

RoomsTabWidget::RoomsTabWidget(QWidget *parent):QTabWidget(parent)
{
	createServerFrame();

	DAmn *damn = new DAmn(this);
	connect(damn, SIGNAL(serverConnected()), this, SLOT(onConnectServer()));
	connect(damn, SIGNAL(textReceived(QString, QString, MessageType, QString, bool)), this, SLOT(onText(QString, QString, MessageType, QString, bool)));
	connect(damn, SIGNAL(usersReceived(QString, QStringList)), this, SLOT(onUsers(QString, QStringList)));
	connect(damn, SIGNAL(roomJoined(QString)), this, SLOT(onJoinRoom(QString)));
	connect(damn, SIGNAL(roomParted(QString, QString)), this, SLOT(onPartRoom(QString, QString)));
	connect(damn, SIGNAL(userJoined(QString, QString, bool)), this, SLOT(onUserJoin(QString, QString, bool)));
	connect(damn, SIGNAL(userParted(QString, QString, QString, bool)), this, SLOT(onUserPart(QString, QString, QString, bool)));
	connect(damn, SIGNAL(userPrivChanged(QString, QString, QString, QString)), this, SLOT(onUserPriv(QString, QString, QString, QString)));
	connect(damn, SIGNAL(errorReceived(QString)), this, SLOT(onError(QString)));
	connect(damn, SIGNAL(authenticationFailed()), this, SLOT(onRequestDAmnToken()));

	OAuth2 *oauth = new OAuth2(this);
	connect(oauth, SIGNAL(errorReceived(QString)), this, SLOT(onError(QString)));
	connect(oauth, SIGNAL(damnTokenReceived(QString, QString)), this, SLOT(onReceiveAuthtoken(QString, QString)));

	connect(this, SIGNAL(currentChanged(int)), this, SLOT(onRoomFocus(int)));
}

RoomsTabWidget::~RoomsTabWidget()
{
}

bool RoomsTabWidget::createServerFrame()
{
	int id = addTab(new ServerFrame(this), tr("Server"));

	setCurrentIndex(id);

	return true;
}

ServerFrame* RoomsTabWidget::getServerFrame()
{
	for(int i = 0; i < count(); ++i)
	{
		ServerFrame *frame = qobject_cast<ServerFrame*>(widget(i));

		if (frame) return frame;
	}

	return NULL;
}

bool RoomsTabWidget::createRoomFrame(const QString &room)
{
	// a tab already exists for this room
	if (getRoomFrame(room)) return false;

	int id = addTab(new RoomFrame(this, room), room);

	setCurrentIndex(id);

	return true;
}

bool RoomsTabWidget::removeRoomFrame(const QString &room)
{
	for(int i = 0; i < count(); ++i)
	{
		RoomFrame *frame = qobject_cast<RoomFrame*>(widget(i));

		if (frame && frame->getRoom() == room)
		{
			removeTab(i);

			delete frame;
		}
	}

	return false;
}

RoomFrame* RoomsTabWidget::getRoomFrame(const QString &room)
{
	for(int i = 0; i < count(); ++i)
	{
		RoomFrame *frame = qobject_cast<RoomFrame*>(widget(i));

		if (frame && frame->getRoom() == room) return frame;
	}

	return NULL;
}

RoomFrame* RoomsTabWidget::getCurrentRoomFrame()
{
	int i = currentIndex();

	if (i < 0) return NULL;

	return qobject_cast<RoomFrame*>(widget(i));
}

void RoomsTabWidget::onConnectServer()
{
	setSystem(tr("Connected to server"));

	ConfigRooms rooms = ConfigFile::getInstance()->getRooms();

	ConfigRoomsIterator it = rooms.begin();

	while(it != rooms.end())
	{
		if (it->autoconnect) DAmn::getInstance()->join(it->name);

		++it;
	}
}

void RoomsTabWidget::onRequestDAmnToken()
{
	QString log = ConfigFile::getInstance()->getLogin();
	QString password = ConfigFile::getInstance()->getPassword();

	// delete previous authtoken because invalid
	ConfigFile::getInstance()->setDAmnToken("");

	if (ConfigFile::getInstance()->isRememberPassword() && !log.isEmpty() && !password.isEmpty())
	{
		if (ConfigFile::getInstance()->getDAmnTokenMethod() == MethodOAuth2)
		{
			OAuth2::getInstance()->login(log, password);
		}
		else
		{
			OAuth2::getInstance()->loginSite(log, password);
		}
	}
	else
	{
		login();
	}
}

void RoomsTabWidget::onReceiveAuthtoken(const QString &login, const QString &authtoken)
{
	ConfigFile::getInstance()->setLogin(login);
	ConfigFile::getInstance()->setDAmnToken(authtoken);

	DAmn::getInstance()->setLogin(login);
	DAmn::getInstance()->setToken(authtoken);

	DAmn::getInstance()->connectToServer();
}

void RoomsTabWidget::onText(const QString &room, const QString &user, MessageType type, const QString &text, bool html)
{
	if (html)
	{
		if (!room.isEmpty())
		{
			RoomFrame *frame = getRoomFrame(room);

			if (frame)
			{
				switch(type)
				{
					case MessageText: frame->setText(user, text); break;
					case MessageAction: if (!text.isEmpty()) frame->setAction(user, text); break;
					case MessageTopic: if (!text.isEmpty()) frame->setSystem(tr("Topic changed by %1: %2").arg(user).arg(text)); break;
					case MessageTitle: if (!text.isEmpty()) frame->setSystem(tr("Title changed by %1: %2").arg(user).arg(text)); break;
					default: break;
				}

				updateSystrayIcon(room, user, text);
			}
		}
		else
		{
			setSystem(text);
		}
	}
	else
	{
		// TODO: write to logs
	}
}

void RoomsTabWidget::onUserJoin(const QString &room, const QString &user, bool show)
{
	RoomFrame *frame = getRoomFrame(room);

	if (frame) frame->userJoin(user);

	updateSystrayIcon(room, user, "");
}

void RoomsTabWidget::onUserPart(const QString &room, const QString &user, const QString &reason, bool show)
{
	RoomFrame *frame = getRoomFrame(room);

	if (frame) frame->userPart(user, reason);

	updateSystrayIcon(room, user, "");
}

void RoomsTabWidget::onUserPriv(const QString &room, const QString &user, const QString &by, const QString &pc)
{
	RoomFrame *frame = getRoomFrame(room);

	if (frame) frame->setSystem(tr("%1 has been made a member of %2 by %3").arg(user).arg(pc).arg(by));
}

void RoomsTabWidget::onUsers(const QString &room, const QStringList &users)
{
	RoomFrame *frame = getRoomFrame(room);

	if (frame) frame->setUsers(users);
}

void RoomsTabWidget::onJoinRoom(const QString &room)
{
	createRoomFrame(room);

	setSystem(tr("You joined room <b>%1</b>").arg(room));

	ConfigFile::getInstance()->setRoomConnected(room, true);
}

void RoomsTabWidget::onPartRoom(const QString &room, const QString &reason)
{
	removeRoomFrame(room);

	QString str = tr("You leaved room <b>%1</b>").arg(room);

	if (!reason.isEmpty()) str += QString(" (%1)").arg(reason);

	setSystem(str);

	ConfigFile::getInstance()->setRoomConnected(room, false);
}

void RoomsTabWidget::onError(const QString &error)
{
	ServerFrame *frame = getServerFrame();

	if (frame) frame->setError(error);

	updateSystrayIcon("", "", "");
}

void RoomsTabWidget::setSystem(const QString &text)
{
//	serverBrowser->append(QString("<div class=\"system\">%1</div>").arg(text));
}

void RoomsTabWidget::onRoomFocus(int index)
{
	for(int i = 0; i < count(); ++i)
	{
		bool focused = index == i;

		RoomFrame *frame = qobject_cast<RoomFrame*>(widget(i));

		if (frame)
		{
			frame->setFocus(focused);

			if (focused)
			{
				SystrayIcon::getInstance()->setStatus(frame->getRoom(), StatusNormal);
				tabBar()->setTabTextColor(i, QColor(QColor::Invalid));
			}
		}
		else
		{
			if (focused)
			{
				SystrayIcon::getInstance()->setStatus("", StatusNormal);
				tabBar()->setTabTextColor(i, QColor(QColor::Invalid));
			}
		}
	}
}

void RoomsTabWidget::updateSystrayIcon(const QString &room, const QString &user, const QString &html)
{
	RoomFrame *frame = NULL;
	int index = 0;
	bool found = false;
	
	for(int i = 0; i < count(); ++i)
	{
		frame = qobject_cast<RoomFrame*>(widget(i));

		if (frame)
		{
			if (frame->getRoom() == room)
			{
				index = i;
				found = true;
				break;
			}
		}
		else
		{
			index = i;
		}
	}

	// server tab
	if (!found)
	{
		if (index == currentIndex()) return;
	}
	else
	{
		if (frame && frame->getFocus()) return;
	}

	QString login = ConfigFile::getInstance()->getLogin().toLower();

	// don't alert if we talk to ourself
	if (login == user.toLower()) return;

	SystrayStatus oldStatus = SystrayIcon::getInstance()->getStatus(room);
	SystrayStatus newStatus = html.toLower().indexOf(login) > -1 ? StatusTalkMe:StatusTalkOther;

	if (newStatus > oldStatus)
	{
		SystrayIcon::getInstance()->setStatus(room, newStatus);
		tabBar()->setTabTextColor(index, newStatus == StatusTalkMe ? Qt::red:Qt::blue);
	}
}

void RoomsTabWidget::login()
{
	QString log = ConfigFile::getInstance()->getLogin();
	QString password = ConfigFile::getInstance()->getPassword();
	QString token = ConfigFile::getInstance()->getDAmnToken();

	DAmn::getInstance()->setLogin(log);
	DAmn::getInstance()->setToken(token);

	if (!DAmn::getInstance()->connectToServer())
	{
		if (ConfigFile::getInstance()->getDAmnTokenMethod() == MethodOAuth2)
		{
			OAuth2::getInstance()->login(log, password);
		}
		else
		{
			OAuth2::getInstance()->loginSite(log, password);
		}
	}
}