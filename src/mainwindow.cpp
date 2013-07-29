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
#include "mainwindow.h"
#include "moc_mainwindow.cpp"
#include "connectdialog.h"
#include "joinchanneldialog.h"
#include "configfile.h"
#include "channelsdialog.h"
#include "channelframe.h"
#include "damn.h"
#include "oauth2.h"
#include "systrayicon.h"

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

MainWindow::MainWindow():QMainWindow()
{
	setupUi(this);

	// Server menu
	connect(actionConnect, SIGNAL(triggered()), this, SLOT(onConnect()));
	connect(actionDisconnect, SIGNAL(triggered()), this, SLOT(onDisconnect()));
	connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));

	// Channel menu
	connect(actionJoin, SIGNAL(triggered()), this, SLOT(onJoin()));
	connect(actionPart, SIGNAL(triggered()), this, SLOT(onPart()));
	connect(actionKnownChannels, SIGNAL(triggered()), this, SLOT(onChannels()));

	// Help menu
	connect(actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
	connect(actionAboutQt, SIGNAL(triggered()), this, SLOT(onAboutQt()));

	connect(channelsWidget, SIGNAL(currentChanged(int)), this, SLOT(onChannelFocus(int)));

	new ConfigFile(this);

	DAmn *damn = new DAmn(this);
	connect(damn, SIGNAL(serverConnected()), this, SLOT(onConnectServer()));
	connect(damn, SIGNAL(textReceived(QString, QString, MessageType, QString, bool)), this, SLOT(onText(QString, QString, MessageType, QString, bool)));
	connect(damn, SIGNAL(usersReceived(QString, QStringList)), this, SLOT(onUsers(QString, QStringList)));
	connect(damn, SIGNAL(channelJoined(QString)), this, SLOT(onJoinChannel(QString)));
	connect(damn, SIGNAL(channelParted(QString, QString)), this, SLOT(onPartChannel(QString, QString)));
	connect(damn, SIGNAL(userJoined(QString, QString, bool)), this, SLOT(onUserJoin(QString, QString, bool)));
	connect(damn, SIGNAL(userParted(QString, QString, QString, bool)), this, SLOT(onUserPart(QString, QString, QString, bool)));
	connect(damn, SIGNAL(userPrivChanged(QString, QString, QString, QString)), this, SLOT(onUserPriv(QString, QString, QString, QString)));
	connect(damn, SIGNAL(errorReceived(QString)), this, SLOT(onError(QString)));
	connect(damn, SIGNAL(authenticationFailed()), this, SLOT(onRequestDAmnToken()));

	OAuth2 *oauth = new OAuth2(this);
	connect(oauth, SIGNAL(errorReceived(QString)), this, SLOT(onError(QString)));
	connect(oauth, SIGNAL(damnTokenReceived(QString, QString)), this, SLOT(onReceiveAuthtoken(QString, QString)));

	new SystrayIcon(this);

	autoConnect();

	connect(qApp, SIGNAL(focusWindowChanged(QWindow*)), this, SLOT(onFocus(QWindow*)));
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	hide();

	event->accept();
}

void MainWindow::onAbout()
{
	QMessageBox::about(this, tr("About %1 %2 by %3").arg(PRODUCT).arg(VERSION).arg(AUTHOR),
		QString("%1<br><br>%2").arg(tr(DESCRIPTION)).arg(tr("deviantART SVG icon by <a href=\"http://abluescarab.deviantart.com\">Alana Gilston</a>")));
}

void MainWindow::onAboutQt()
{
	QMessageBox::aboutQt(this);
}

void MainWindow::onConnect()
{
	ConnectDialog dialog(this);

	if (dialog.exec())
	{
		QString login = dialog.getLogin();
		QString password = dialog.getPassword();
		QString token = dialog.getToken();
		bool remember = dialog.isRememberPassword();

		ConfigFile::getInstance()->setLogin(login);
		ConfigFile::getInstance()->setPassword(remember ? password:"");
		ConfigFile::getInstance()->rememberPassword(remember);
		ConfigFile::getInstance()->setDAmnToken(token);

		DAmn::getInstance()->setLogin(login);
		DAmn::getInstance()->setToken(token);

		if (!DAmn::getInstance()->connectToServer())
		{
			OAuth2::getInstance()->login(login, password);
//			OAuth2::getInstance()->loginSite(login, password);
		}
	}
}

void MainWindow::onDisconnect()
{
	DAmn::getInstance()->disconnect();
}

void MainWindow::onReceiveAuthtoken(const QString &login, const QString &authtoken)
{
	ConfigFile::getInstance()->setLogin(login);
	ConfigFile::getInstance()->setDAmnToken(authtoken);

	DAmn::getInstance()->setLogin(login);
	DAmn::getInstance()->setToken(authtoken);

	DAmn::getInstance()->connectToServer();
}

void MainWindow::onJoin()
{
	JoinChannelDialog dialog(this);

	if (dialog.exec())
	{
		QString channel = dialog.getChannel();

		DAmn::getInstance()->join(channel);

		ConfigFile::getInstance()->setChannelAutoConnect(channel, true);
	}
}

void MainWindow::onPart()
{
	ChannelFrame *frame = getCurrentChannelFrame();

	if (frame)
	{
		DAmn::getInstance()->part(frame->getChannel());

		ConfigFile::getInstance()->setChannelAutoConnect(frame->getChannel(), false);
	}
}

void MainWindow::onChannels()
{
	ChannelsDialog dialog(this);

	if (dialog.exec())
	{
	}
}

void MainWindow::onConnectServer()
{
	setSystem(tr("Connected to server"));

	ConfigChannels channels = ConfigFile::getInstance()->getChannels();

	ConfigChannelsIterator it = channels.begin();

	while(it != channels.end())
	{
		if (it->autoconnect) DAmn::getInstance()->join(it->name);

		++it;
	}
}

void MainWindow::onText(const QString &channel, const QString &user, MessageType type, const QString &text, bool html)
{
	if (html)
	{
		if (!channel.isEmpty())
		{
			ChannelFrame *frame = getChannelFrame(channel);

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

				updateSystrayIcon(channel, user, text);
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

void MainWindow::onUserJoin(const QString &channel, const QString &user, bool show)
{
	ChannelFrame *frame = getChannelFrame(channel);

	if (frame) frame->userJoin(user);

	updateSystrayIcon(channel, user, "");
}

void MainWindow::onUserPart(const QString &channel, const QString &user, const QString &reason, bool show)
{
	ChannelFrame *frame = getChannelFrame(channel);

	if (frame) frame->userPart(user, reason);

	updateSystrayIcon(channel, user, "");
}

void MainWindow::onUserPriv(const QString &channel, const QString &user, const QString &by, const QString &pc)
{
	ChannelFrame *frame = getChannelFrame(channel);

	if (frame) frame->setSystem(tr("%1 has been made a member of %2 by %3").arg(user).arg(pc).arg(by));
}

void MainWindow::onUsers(const QString &channel, const QStringList &users)
{
	ChannelFrame *frame = getChannelFrame(channel);

	if (frame) frame->setUsers(users);
}

void MainWindow::onJoinChannel(const QString &channel)
{
	createChannelFrame(channel);

	setSystem(tr("You joined channel <b>%1</b>").arg(channel));

	ConfigFile::getInstance()->setChannelConnected(channel, true);
}

void MainWindow::onPartChannel(const QString &channel, const QString &reason)
{
	removeChannelFrame(channel);

	QString str = tr("You leaved channel <b>%1</b>").arg(channel);

	if (!reason.isEmpty()) str += QString(" (%1)").arg(reason);

	setSystem(str);

	ConfigFile::getInstance()->setChannelConnected(channel, false);
}

void MainWindow::onError(const QString &error)
{
	serverBrowser->append(QString("<div class=\"error\">%1</div>").arg(error));

	updateSystrayIcon("", "", "");
}

bool MainWindow::createChannelFrame(const QString &channel)
{
	// a tab already exists for this channel
	if (getChannelFrame(channel)) return false;

	int id = channelsWidget->addTab(new ChannelFrame(this, channel), channel);

	channelsWidget->setCurrentIndex(id);

	return true;
}

bool MainWindow::removeChannelFrame(const QString &channel)
{
	for(int i = 0; i < channelsWidget->count(); ++i)
	{
		ChannelFrame *frame = qobject_cast<ChannelFrame*>(channelsWidget->widget(i));

		if (frame && frame->getChannel() == channel)
		{
			channelsWidget->removeTab(i);

			delete frame;
		}
	}

	return false;
}

ChannelFrame* MainWindow::getChannelFrame(const QString &channel)
{
	for(int i = 0; i < channelsWidget->count(); ++i)
	{
		if (channelsWidget->tabText(i) == channel)
		{
			return qobject_cast<ChannelFrame*>(channelsWidget->widget(i));
		}
	}

	return NULL;
}

ChannelFrame* MainWindow::getCurrentChannelFrame()
{
	int i = channelsWidget->currentIndex();

	if (i < 0) return NULL;

	return qobject_cast<ChannelFrame*>(channelsWidget->widget(i));
}

void MainWindow::setSystem(const QString &text)
{
	serverBrowser->append(QString("<div class=\"system\">%1</div>").arg(text));
}

void MainWindow::trayActivated(QSystemTrayIcon::ActivationReason reseaon)
{
}

void MainWindow::onChannelFocus(int index)
{
	for(int i = 0; i < channelsWidget->count(); ++i)
	{
		bool focused = index == i;

		ChannelFrame *frame = qobject_cast<ChannelFrame*>(channelsWidget->widget(i));

		if (frame)
		{
			frame->setFocus(focused);

			if (focused)
			{
				SystrayIcon::getInstance()->setStatus(frame->getChannel(), StatusNormal);
				channelsWidget->tabBar()->setTabTextColor(i, QColor(QColor::Invalid));
			}
		}
		else
		{
			if (focused)
			{
				SystrayIcon::getInstance()->setStatus("", StatusNormal);
				channelsWidget->tabBar()->setTabTextColor(i, QColor(QColor::Invalid));
			}
		}
	}
}

void MainWindow::onFocus(QWindow *window)
{
	onChannelFocus(window && window->isActive() ? channelsWidget->currentIndex():-1);
}

void MainWindow::autoConnect()
{
	QString login = ConfigFile::getInstance()->getLogin();
	QString token = ConfigFile::getInstance()->getDAmnToken();

	if (!login.isEmpty() && !token.isEmpty())
	{
		DAmn::getInstance()->setLogin(login);
		DAmn::getInstance()->setToken(token);

		DAmn::getInstance()->connectToServer();
	}
}

void MainWindow::onRequestDAmnToken()
{
	QString login = ConfigFile::getInstance()->getLogin();
	QString password = ConfigFile::getInstance()->getPassword();

	// delete previous authtoken because invalid
	ConfigFile::getInstance()->setDAmnToken("");

	if (ConfigFile::getInstance()->isRememberPassword() && !login.isEmpty() && !password.isEmpty())
	{
		OAuth2::getInstance()->login(login, password);
	}
	else
	{
		onConnect();
	}
}

void MainWindow::updateSystrayIcon(const QString &channel, const QString &user, const QString &html)
{
	ChannelFrame *frame = NULL;
	int index = 0;
	bool found = false;
	
	for(int i = 0; i < channelsWidget->count(); ++i)
	{
		frame = qobject_cast<ChannelFrame*>(channelsWidget->widget(i));

		if (frame)
		{
			if (frame->getChannel() == channel)
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
		if (index == channelsWidget->currentIndex()) return;
	}
	else
	{
		if (frame && frame->getFocus()) return;
	}

	QString login = ConfigFile::getInstance()->getLogin().toLower();

	// don't alert if we talk to ourself
	if (login == user.toLower()) return;

	SystrayStatus oldStatus = SystrayIcon::getInstance()->getStatus(channel);
	SystrayStatus newStatus = html.toLower().indexOf(login) > -1 ? StatusTalkMe:StatusTalkOther;

	if (newStatus > oldStatus)
	{
		SystrayIcon::getInstance()->setStatus(channel, newStatus);
		channelsWidget->tabBar()->setTabTextColor(index, newStatus == StatusTalkMe ? Qt::red:Qt::blue);
	}
}
