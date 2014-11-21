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

#include "common.h"
#include "roomstabwidget.h"
#include "moc_roomstabwidget.cpp"
#include "damn.h"
#include "oauth2.h"
#include "roomframe.h"
#include "serverframe.h"
#include "notesframe.h"
#include "noteframe.h"
#include "configfile.h"
#include "systrayicon.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

static QString base36enc(qint64 value)
{
	static const QString base36("0123456789abcdefghijklmnopqrstuvwxyz");

	QString res;

	do
	{
		res.prepend(base36[(int)(value % 36)]);
	}
	while (value /= 36);

	return res;
}

RoomsTabWidget::RoomsTabWidget(QWidget *parent):QTabWidget(parent), m_messagesTimer(NULL)
{
	createServerFrame();

	DAmn *damn = new DAmn(this);
	connect(damn, SIGNAL(serverConnected()), this, SLOT(onConnectServer()));
	connect(damn, SIGNAL(textReceived(QString, QString, EMessageType, QString, bool)), this, SLOT(onText(QString, QString, EMessageType, QString, bool)));
	connect(damn, SIGNAL(usersReceived(QString, QStringList)), this, SLOT(onUsers(QString, QStringList)));
	connect(damn, SIGNAL(roomJoined(QString)), this, SLOT(onJoinRoom(QString)));
	connect(damn, SIGNAL(roomParted(QString, QString)), this, SLOT(onPartRoom(QString, QString)));
	connect(damn, SIGNAL(userJoined(QString, QString, bool)), this, SLOT(onUserJoin(QString, QString, bool)));
	connect(damn, SIGNAL(userParted(QString, QString, QString, bool)), this, SLOT(onUserPart(QString, QString, QString, bool)));
	connect(damn, SIGNAL(userKicked(QString, QString, QString)), this, SLOT(onUserKick(QString, QString, QString)));
	connect(damn, SIGNAL(userPrivChanged(QString, QString, QString, QString)), this, SLOT(onUserPriv(QString, QString, QString, QString)));
	connect(damn, SIGNAL(errorReceived(QString)), this, SLOT(onError(QString)));
//	connect(damn, SIGNAL(authenticationFailedWrongLogin()), this, SLOT(onRequestDAmnToken()));
	connect(damn, SIGNAL(authenticationFailedWrongToken()), this, SLOT(onRequestDAmnToken()));

	OAuth2 *oauth = new OAuth2(this);
	connect(oauth, SIGNAL(loggedIn()), this, SLOT(onLoggedIn()));
	connect(oauth, SIGNAL(errorReceived(QString)), this, SLOT(onError(QString)));
	connect(oauth, SIGNAL(damnTokenReceived(QString, QString)), this, SLOT(onReceiveDAmnToken(QString, QString)));
	connect(oauth, SIGNAL(accessTokenReceived(QString, QString)), this, SLOT(onReceiveAccessToken(QString, QString)));
	connect(oauth, SIGNAL(imageUploaded(QString, QString)), this, SLOT(onUploadImage(QString, QString)));
	connect(oauth, SIGNAL(notesReceived(int)), this, SLOT(onReceiveNotes(int)));
	connect(oauth, SIGNAL(notesUpdated(QString, int, int)), this, SLOT(onUpdateNotes(QString, int, int)));
	connect(oauth, SIGNAL(notePrepared()), this, SLOT(onPrepareNote()));
	connect(oauth, SIGNAL(noteSent(QString)), this, SLOT(onSendNote(QString)));

	m_messagesTimer = new QTimer(this);
	m_messagesTimer->setSingleShot(true);
	connect(m_messagesTimer, SIGNAL(timeout()), this, SLOT(checkMessages()));

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

bool RoomsTabWidget::createNotesFrame()
{
	int id = addTab(new NotesFrame(this), tr("Notes"));

	setCurrentIndex(id);

	return true;
}

bool RoomsTabWidget::removeNotesFrame()
{
	for(int i = 0; i < count(); ++i)
	{
		NotesFrame *frame = qobject_cast<NotesFrame*>(widget(i));

		if (frame)
		{
			removeTab(i);

			delete frame;

			return true;
		}
	}

	return false;
}

NotesFrame* RoomsTabWidget::getNotesFrame()
{
	for(int i = 0; i < count(); ++i)
	{
		NotesFrame *frame = qobject_cast<NotesFrame*>(widget(i));

		if (frame) return frame;
	}

	return NULL;
}

bool RoomsTabWidget::createNoteFrame()
{
	int id = addTab(new NoteFrame(this), tr("Note"));

	setCurrentIndex(id);

	return true;
}

bool RoomsTabWidget::removeNoteFrame()
{
	for(int i = 0; i < count(); ++i)
	{
		NoteFrame *frame = qobject_cast<NoteFrame*>(widget(i));

		if (frame)
		{
			removeTab(i);

			delete frame;

			return true;
		}
	}

	return false;
}

NoteFrame* RoomsTabWidget::getNoteFrame()
{
	for(int i = 0; i < count(); ++i)
	{
		NoteFrame *frame = qobject_cast<NoteFrame*>(widget(i));

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

			return true;
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

	// we need to log to DA site for OAuth2 and DiFi requests
	OAuth2::getInstance()->login();
}

void RoomsTabWidget::onRequestDAmnToken()
{
	QString user = ConfigFile::getInstance()->getLogin();
	QString password = ConfigFile::getInstance()->getPassword();

	// delete previous dAmn token because invalid
	ConfigFile::getInstance()->setDAmnToken("");
	OAuth2::getInstance()->setDAmnToken("");
	DAmn::getInstance()->setToken("");

	if (ConfigFile::getInstance()->isRememberPassword() && !user.isEmpty() && !password.isEmpty())
	{
		OAuth2::getInstance()->requestDAmnToken();
	}
	else
	{
		login();
	}
}

void RoomsTabWidget::onLoggedIn()
{
	m_messagesTimer->start(10000);
}

void RoomsTabWidget::onReceiveDAmnToken(const QString &login, const QString &authtoken)
{
	ConfigFile::getInstance()->setLogin(login);
	ConfigFile::getInstance()->setDAmnToken(authtoken);

	DAmn::getInstance()->setLogin(login);
	DAmn::getInstance()->setToken(authtoken);

	DAmn::getInstance()->connectToServer();
}

void RoomsTabWidget::onReceiveAccessToken(const QString &access, const QString &refresh)
{
	ConfigFile::getInstance()->setAccessToken(access);
	ConfigFile::getInstance()->setRefreshToken(refresh);
}

void RoomsTabWidget::onUploadImage(const QString &room, const QString &stashId)
{
	QString base36StashId = base36enc(stashId.toLongLong());

	QString url = QString("http://sta.sh/0%1").arg(base36StashId);

	DAmn::getInstance()->send(room, url);
//	OAuth2::getInstance()->requestImageInfo(url, "");
}

void RoomsTabWidget::onReceiveNotes(int count)
{
	static int lastCount = 0;

	if (count != lastCount)
	{
		if (count > 0)
		{
			setSystem(tr("You received %n note(s), click <a href=\"https://www.deviantart.com/messages/notes/\">here</a> to read them.", "", count));

			// display message in system tab
			updateSystrayIcon("", "", ConfigFile::getInstance()->getLogin().toLower());

			SystrayIcon::getInstance()->displayMessage(tr("You received %n note(s), click to read them", "", count), "https://www.deviantart.com/messages/notes/");
		}

		lastCount = count;
	}

	m_messagesTimer->start(10000);
}

void RoomsTabWidget::onUpdateNotes(const QString &folderId, int offset, int count)
{
	NotesFrame *frame = getNotesFrame();

	if (!frame)
	{
		// frame not yet created, create it
		createNotesFrame();

		frame = getNotesFrame();
	}

	if (frame)
	{
		const Notes &notes = OAuth2::getInstance()->getFolder(folderId).notes;

		if (frame->getCurrentFolderId() != folderId)
		{
			frame->setNotes(notes);
		}
		else
		{
			frame->updateNotes(notes, offset, count);
		}
	}
}

void RoomsTabWidget::onPrepareNote()
{
	NoteFrame *frame = getNoteFrame();

	if (!frame)
	{
		// frame not yet created, create it
		createNoteFrame();
	}
}

void RoomsTabWidget::onSendNote(const QString &noteId)
{
	removeNoteFrame();
}

void RoomsTabWidget::checkMessages()
{
	OAuth2::getInstance()->requestMessageViews();
}

void RoomsTabWidget::onText(const QString &room, const QString &user, EMessageType type, const QString &text, bool html)
{
	if (!room.isEmpty())
	{
		RoomFrame *frame = getRoomFrame(room);

		if (frame)
		{
			switch(type)
			{
				case MessageText: frame->setText(user, text, html); break;
				case MessageAction: frame->setAction(user, text, html); break;
				case MessageTopic: frame->setTopic(user, text, html); break;
				case MessageTitle: frame->setTitle(user, text, html); break;
				default: break;
			}
		}
		else
		{
			onError(tr("Room %1 doesn't exist").arg(room));
		}

		if (!html) updateSystrayIcon(room, user, text);
	}
	else
	{
		setSystem(text);
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

void RoomsTabWidget::onUserKick(const QString &room, const QString &user, const QString &by)
{
	RoomFrame *frame = getRoomFrame(room);

	if (frame) frame->userPart(user, tr("%1 has been kicked by %2").arg(user).arg(by));

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

	setSystem(tr("You joined room %1").arg(room));

	ConfigFile::getInstance()->setRoomConnected(room, true);
}

void RoomsTabWidget::onPartRoom(const QString &room, const QString &reason)
{
	removeRoomFrame(room);

	QString str = tr("You left room %1").arg(room);

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
	ServerFrame *frame = getServerFrame();

	if (frame) frame->setSystem(text);
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

void RoomsTabWidget::updateSystrayIcon(const QString &room, const QString &user, const QString &text)
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
	SystrayStatus newStatus = text.toLower().indexOf(login) > -1 ? StatusTalkMe:StatusTalkOther;

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
	QString damnToken = ConfigFile::getInstance()->getDAmnToken();
	QString accessToken = ConfigFile::getInstance()->getAccessToken();
	QString refreshToken = ConfigFile::getInstance()->getRefreshToken();

	OAuth2::getInstance()->setLogin(log);
	OAuth2::getInstance()->setPassword(password);
	OAuth2::getInstance()->setDAmnToken(damnToken);
	OAuth2::getInstance()->setAccessToken(accessToken, refreshToken);

	DAmn::getInstance()->setLogin(log);
	DAmn::getInstance()->setToken(damnToken);

	if (!DAmn::getInstance()->connectToServer())
	{
		OAuth2::getInstance()->requestDAmnToken();
	}
}
