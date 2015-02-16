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
#include "roomstabwidget.h"
#include "moc_roomstabwidget.cpp"
#include "damn.h"
#include "damnuser.h"
#include "oauth2.h"
#include "roomframe.h"
#include "serverframe.h"
#include "notesframe.h"
#include "noteframe.h"
#include "configfile.h"
#include "systrayicon.h"
#include "htmlformatting.h"
#include "utils.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

RoomsTabWidget::RoomsTabWidget(QWidget *parent):QTabWidget(parent), m_messagesTimer(NULL), m_reconnectTimer(NULL)
{
	updateScreenCss();
	updateLogCss();

	createServerFrame();

	m_formatHtml = new HtmlFormatting(true, this);
	m_formatText = new HtmlFormatting(false, this);

	DAmn *damn = new DAmn(this);
	connect(damn, SIGNAL(serverConnected()), this, SLOT(onConnectServer()));
	connect(damn, SIGNAL(serverDisconnected(bool)), this, SLOT(onDisconnectServer(bool)));
	connect(damn, SIGNAL(textReceived(QString, QString, EMessageType, QString, bool)), this, SLOT(onText(QString, QString, EMessageType, QString, bool)));
	connect(damn, SIGNAL(usersReceived(QString, QStringList)), this, SLOT(onUsers(QString, QStringList)));
	connect(damn, SIGNAL(roomJoined(QString)), this, SLOT(onJoinRoom(QString)));
	connect(damn, SIGNAL(roomParted(QString, QString)), this, SLOT(onPartRoom(QString, QString)));
	connect(damn, SIGNAL(userJoined(QString, QString, bool)), this, SLOT(onUserJoin(QString, QString, bool)));
	connect(damn, SIGNAL(userParted(QString, QString, QString, bool)), this, SLOT(onUserPart(QString, QString, QString, bool)));
	connect(damn, SIGNAL(userKicked(QString, QString, QString)), this, SLOT(onUserKick(QString, QString, QString)));
	connect(damn, SIGNAL(userPrivChanged(QString, QString, QString, QString)), this, SLOT(onUserPriv(QString, QString, QString, QString)));
	connect(damn, SIGNAL(privClassChanged(QString, QString, QString, QString)), this, SLOT(onPrivClass(QString, QString, QString, QString)));
	connect(damn, SIGNAL(errorReceived(QString)), this, SLOT(onError(QString)));
//	connect(damn, SIGNAL(authenticationFailedWrongLogin()), this, SLOT(onRequestDAmnToken()));
	connect(damn, SIGNAL(authenticationFailedWrongToken()), this, SLOT(onRequestDAmnToken()));

	OAuth2 *oauth = OAuth2::getInstance();
	connect(oauth, SIGNAL(foldersReceived()), this, SLOT(onReceiveFolders()));
	connect(oauth, SIGNAL(loggedIn()), this, SLOT(onLoggedIn()));
	connect(oauth, SIGNAL(errorReceived(QString)), this, SLOT(onError(QString)));
	connect(oauth, SIGNAL(damnTokenReceived(QString, QString)), this, SLOT(onReceiveDAmnToken(QString, QString)));
	connect(oauth, SIGNAL(imageUploaded(QString, QString)), this, SLOT(onUploadImage(QString, QString)));
	connect(oauth, SIGNAL(notesReceived(int)), this, SLOT(onReceiveNotes(int)));
	connect(oauth, SIGNAL(notesUpdated(QString, int, int)), this, SLOT(onUpdateNotes(QString, int, int)));
	connect(oauth, SIGNAL(noteUpdated(QString, QString)), this, SLOT(onUpdateNote(QString, QString)));

	connect(oauth, SIGNAL(notePrepared()), this, SLOT(onPrepareNote()));
	connect(oauth, SIGNAL(noteSent(QString)), this, SLOT(onSendNote(QString)));

	m_messagesTimer = new QTimer(this);
	m_messagesTimer->setSingleShot(true);
	connect(m_messagesTimer, SIGNAL(timeout()), this, SLOT(checkMessages()));

	m_reconnectTimer = new QTimer(this);
	m_reconnectTimer->setSingleShot(true);
	connect(m_reconnectTimer, SIGNAL(timeout()), this, SLOT(onReconnectServer()));

	connect(this, SIGNAL(currentChanged(int)), this, SLOT(onRoomFocus(int)));
}

RoomsTabWidget::~RoomsTabWidget()
{
}

bool RoomsTabWidget::createServerFrame()
{
	ServerFrame *frame = new ServerFrame(this);
	frame->updateCssScreen(m_screenCss);

	int id = addTab(frame, tr("Server"));

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

	RoomFrame *frame = new RoomFrame(this, room);
	frame->updateCssScreen(m_screenCss);

	int id = addTab(frame, room);

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
	// stop reconnect timer
	m_reconnectTimer->stop();

	setServer(tr("Connected to server"));

	ConfigRooms rooms = ConfigFile::getInstance()->getRooms();

	ConfigRoomsIterator it = rooms.begin();

	while(it != rooms.end())
	{
		if (it->autoconnect) DAmn::getInstance()->join(it->name);

		++it;
	}

	// we need to log to DA site for OAuth2 and DiFi requests
	OAuth2::getInstance()->requestAuthorization();
}

void RoomsTabWidget::onDisconnectServer(bool reconnect)
{
	setServer(tr("Disconnected from server"));

	if (reconnect) onReconnectServer();
}

void RoomsTabWidget::onReconnectServer()
{
	if (!DAmn::getInstance()->isConnected())
	{
		setServer(tr("Trying to reconnect..."));

		m_reconnectTimer->start(5000);

		DAmn::getInstance()->connectToServer();
	}
}

void RoomsTabWidget::onRequestDAmnToken()
{
	QString user = ConfigFile::getInstance()->getLogin();
	QString password = ConfigFile::getInstance()->getPassword();

	// delete previous dAmn token because invalid
	ConfigFile::getInstance()->setDAmnToken("");
	OAuth2::getInstance()->setDAmnToken("");
	DAmn::getInstance()->setToken("");

	if (ConfigFile::getInstance()->getRememberPassword() && !user.isEmpty() && !password.isEmpty())
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
	OAuth2::getInstance()->requestMessageCenterGetFolders();
}

void RoomsTabWidget::onReceiveFolders()
{
	// begin to check for notes
	m_messagesTimer->start(ConfigFile::getInstance()->getCheckMessagesDelay() * 1000);
}

void RoomsTabWidget::onReceiveDAmnToken(const QString &login, const QString &authtoken)
{
	ConfigFile::getInstance()->setLogin(login);
	ConfigFile::getInstance()->setDAmnToken(authtoken);

	DAmn::getInstance()->setLogin(login);
	DAmn::getInstance()->setToken(authtoken);

	if (!DAmn::getInstance()->isConnected()) DAmn::getInstance()->connectToServer();
}

void RoomsTabWidget::onUploadImage(const QString &room, const QString &url)
{
	DAmn::getInstance()->send(room, url);
}

void RoomsTabWidget::onReceiveNotes(int count)
{
	static int lastCount = 0;

	if (count != lastCount)
	{
		if (count > 0)
		{
			setServer(tr("You received %n note(s), click <a href=\"https://www.deviantart.com/messages/notes/\">here</a> to read them.", "", count));

			// display message in system tab
			updateSystrayIcon("", "", ConfigFile::getInstance()->getLogin().toLower());

			SystrayIcon::getInstance()->displayMessage(tr("You received %n note(s), click to read them", "", count), "https://www.deviantart.com/messages/notes/");
		}

		lastCount = count;
	}

	m_messagesTimer->start(ConfigFile::getInstance()->getCheckMessagesDelay() * 1000);
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
		const Folder &folder = OAuth2::getInstance()->getFolder(folderId);

		if (frame->getCurrentFolderId() != folderId)
		{
			frame->setFolder(folder);
		}
		else
		{
			frame->updateFolder(folder, offset, count);
		}
	}
}

void RoomsTabWidget::onUpdateNote(const QString &folderId, const QString &noteId)
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
		const Folder &folder = OAuth2::getInstance()->getFolder(folderId);

		if (frame->getCurrentFolderId() == folderId)
		{
			Note note;
			note.id = noteId;
			
			int pos = folder.notes.indexOf(note);

			if (pos > -1)
			{
				note = folder.notes[pos];

				frame->updateNote(note);
			}
		}
		else
		{
			// TODO: note in another folder
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
	OAuth2::getInstance()->requestMessageCenterGetViews();
}

void RoomsTabWidget::onText(const QString &room, const QString &user, EMessageType type, const QString &text, bool html)
{
	if (!room.isEmpty())
	{
		RoomFrame *frame = getRoomFrame(room);

		if (frame)
		{
			QString formatted;

			switch(type)
			{
				case MessageText:
				formatted = html ? m_formatHtml->formatMessageText(user, text):m_formatText->formatMessageText(user, text);
				break;

				case MessageAction:
				formatted = html ? m_formatHtml->formatMessageAction(user, text):m_formatText->formatMessageAction(user, text);
				break;

				case MessageTopic:
				formatted = html ? m_formatHtml->formatMessageTopic(user, text):m_formatText->formatMessageTopic(user, text);
				break;

				case MessageTitle:
				formatted = html ? m_formatHtml->formatMessageTitle(user, text):m_formatText->formatMessageTitle(user, text);
				break;

				case MessageTopicFirst:
				formatted = html ? m_formatHtml->formatMessageTopicFirst(user, text):m_formatText->formatMessageTopicFirst(user, text);
				break;

				case MessageTitleFirst:
				formatted = html ? m_formatHtml->formatMessageTitleFirst(user, text):m_formatText->formatMessageTitleFirst(user, text);
				break;

				default:
				break;
			}

			bool display = true;

			// don't display topic or title the first time if empty
			if ((type == MessageTopicFirst || type == MessageTitleFirst) && text.isEmpty()) display = false;

			if (display)
			{
				if (html)
				{
					// first HTML message received
					if (type == MessageText || type == MessageAction) frame->setReceiveHtml(true);

					frame->appendHtml(formatted);
				}
				else
				{
					// first text message received
					if (type == MessageText || type == MessageAction) frame->setReceiveText(true);

					frame->appendText(formatted);
				}
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
		setServer(text);
	}
}

void RoomsTabWidget::onUserJoin(const QString &room, const QString &user, bool show)
{
	RoomFrame *frame = getRoomFrame(room);

	if (frame)
	{
		frame->userJoin(user);

		frame->appendHtml(m_formatHtml->formatUserJoin(user));
		frame->appendText(m_formatText->formatUserJoin(user));
	}

	updateSystrayIcon(room, user, "");
}

void RoomsTabWidget::onUserPart(const QString &room, const QString &user, const QString &reason, bool show)
{
	RoomFrame *frame = getRoomFrame(room);

	if (frame)
	{
		frame->userPart(user);

		frame->appendHtml(m_formatHtml->formatUserPart(user, reason));
		frame->appendText(m_formatText->formatUserPart(user, reason));
	}

	updateSystrayIcon(room, user, "");
}

void RoomsTabWidget::onUserKick(const QString &room, const QString &user, const QString &by)
{
	RoomFrame *frame = getRoomFrame(room);

	if (frame)
	{
		frame->userPart(user);

		frame->appendHtml(m_formatHtml->formatUserKick(user, by));
		frame->appendText(m_formatText->formatUserKick(user, by));
	}

	updateSystrayIcon(room, user, "");
}

void RoomsTabWidget::onUserPriv(const QString &room, const QString &user, const QString &by, const QString &pc)
{
	RoomFrame *frame = getRoomFrame(room);

	if (frame)
	{
		frame->appendHtml(m_formatHtml->formatUserPriv(user, by, pc));
		frame->appendText(m_formatText->formatUserPriv(user, by, pc));
	}
}

void RoomsTabWidget::onPrivClass(const QString &room, const QString &privclass, const QString &by, const QString &privs)
{
	RoomFrame *frame = getRoomFrame(room);

	if (frame)
	{
		frame->appendHtml(m_formatHtml->formatPrivClass(privclass, by, privs));
		frame->appendText(m_formatText->formatPrivClass(privclass, by, privs));
	}
}

void RoomsTabWidget::onUsers(const QString &room, const QStringList &users)
{
	RoomFrame *frame = getRoomFrame(room);

	if (frame) frame->setUsers(users);
}

void RoomsTabWidget::onJoinRoom(const QString &room)
{
	createRoomFrame(room);

	ServerFrame *frame = getServerFrame();

	if (frame)
	{
		frame->appendHtml(m_formatHtml->formatJoinRoom(room));
	}

	ConfigFile::getInstance()->setRoomConnected(room, true);
}

void RoomsTabWidget::onPartRoom(const QString &room, const QString &reason)
{
	removeRoomFrame(room);

	ServerFrame *frame = getServerFrame();

	if (frame)
	{
		frame->appendHtml(m_formatHtml->formatPartRoom(room, reason));
	}

	ConfigFile::getInstance()->setRoomConnected(room, false);
}

void RoomsTabWidget::onError(const QString &error)
{
	ServerFrame *frame = getServerFrame();

	if (frame)
	{
		frame->appendHtml(m_formatHtml->formatLineError(error));
	}

	updateSystrayIcon("", "", "");
}

void RoomsTabWidget::setServer(const QString &text)
{
	ServerFrame *frame = getServerFrame();

	if (frame)
	{
		frame->appendHtml(m_formatHtml->formatLineNormal(text));
	}
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
	SystrayStatus newStatus = m_formatText->searchUser(login, text) ? StatusTalkMe:StatusTalkOther;

	if (newStatus == StatusTalkMe)
	{
		playSound(ConfigFile::getInstance()->getNameMentionedSound());
	}

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

	OAuth2::getInstance()->setLogin(log);
	OAuth2::getInstance()->setPassword(password);
	OAuth2::getInstance()->setDAmnToken(damnToken);

	DAmn::getInstance()->setLogin(log);
	DAmn::getInstance()->setToken(damnToken);

	if (!DAmn::getInstance()->isConnected() && !DAmn::getInstance()->connectToServer())
	{
		OAuth2::getInstance()->requestDAmnToken();
	}
	else
	{
		OAuth2::getInstance()->login();
	}
}

void RoomsTabWidget::updateConfig()
{
	// config file possibly updated
	bool logs = ConfigFile::getInstance()->getEnableLogs();
	bool html = logs && ConfigFile::getInstance()->getEnableHtmlLogs();
	bool text = logs && ConfigFile::getInstance()->getEnableTextLogs();

	// update CSS
	updateScreenCss();
	updateLogCss();

	for(int i = 0; i < count(); ++i)
	{
		RoomFrame *roomFrame = qobject_cast<RoomFrame*>(widget(i));

		if (roomFrame)
		{
			if (!html) roomFrame->closeHtmlLog();
			if (!text) roomFrame->closeTextLog();

			roomFrame->updateCssScreen(m_screenCss);
			roomFrame->updateCssFile(m_logCss);
		}

		ServerFrame *serverFrame = qobject_cast<ServerFrame*>(widget(i));

		if (serverFrame)
		{
			serverFrame->updateCssScreen(m_screenCss);
		}
	}
}

void RoomsTabWidget::updateScreenCss()
{
	QString css;
	QString style = ConfigFile::getInstance()->getScreenStyle();

	// try to load CSS from style
	if (!style.isEmpty())
	{
		css = getCssFromStyle(style);
	}

	// load default CSS
	if (css.isEmpty()) css = getCssFromStyle("screen_default");

	if (css.isEmpty()) return;

	QMap<QString, QColor> colorsMap;

	colorsMap["normal_text_color"] = palette().windowText().color();
	colorsMap["normal_window_color"] = palette().window().color();

	colorsMap["darker_text_color"] = average(colorsMap["normal_text_color"], colorsMap["normal_window_color"], 0.75);
	colorsMap["darkest_text_color"] = average(colorsMap["normal_text_color"], colorsMap["normal_window_color"], 0.5);

	colorsMap["highlight_color"] = average(colorsMap["normal_text_color"], ConfigFile::getInstance()->getHighlightColor(), 0.5); // blue
	colorsMap["error_color"] = average(colorsMap["normal_text_color"], ConfigFile::getInstance()->getErrorColor(), 0.5); // red

	// replace named colors
	QMap<QString, QColor>::const_iterator it = colorsMap.begin(), iend = colorsMap.end();

	while(it != iend)
	{
		css.replace(QString("$%1$").arg(it.key()), it.value().name());

		++it;
	}

	m_screenCss = css;
}

void RoomsTabWidget::updateLogCss()
{
	QString css;
	QString style = ConfigFile::getInstance()->getLogStyle();

	// try to load CSS from style
	if (!style.isEmpty())
	{
		css = getCssFromStyle(style);
	}

	// load default CSS
	if (css.isEmpty()) css = getCssFromStyle("log_default");

	m_logCss = css;
}

void RoomsTabWidget::playSound(const QString &filename) const
{
	if (!ConfigFile::getInstance()->getEnableSound()) return;

	QString sound = ":/icons/default.wav";

	if (!filename.isEmpty() && QFile::exists(filename)) sound = filename;

	QSound::play(sound);
}

QString RoomsTabWidget::getCssFromStyle(const QString &style) const
{
	// partial filename, try to complete it with local data
	QString fullPath = QString("%1/styles/%2.css").arg(ConfigFile::getInstance()->getLocalDataDirectory()).arg(style);

	if (!QFile::exists(fullPath))
	{
		// partial filename, try to complete it with global data
		fullPath = QString("%1/styles/%2.css").arg(ConfigFile::getInstance()->getGlobalDataDirectory()).arg(style);

		if (!QFile::exists(fullPath))
		{
			// not found
			fullPath.clear();
		}
	}

	if (!fullPath.isEmpty())
	{
		QFile file(fullPath);

		if (file.open(QFile::ReadOnly))
		{
			return QString::fromUtf8(file.readAll());
		}
	}

	return QString();
}

void RoomsTabWidget::changeEvent(QEvent *e)
{
	QTabWidget::changeEvent(e);

	if (e->type() == QEvent::PaletteChange)
	{
		updateConfig();
	}
}
