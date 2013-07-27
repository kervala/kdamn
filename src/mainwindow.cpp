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
#include "channelframe.h"
#include "damn.h"
#include "oauth2.h"

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

MainWindow::MainWindow():QMainWindow(), m_trayIcon(NULL)
{
	setupUi(this);

	// Server menu
	connect(actionConnect, SIGNAL(triggered()), this, SLOT(onConnect()));
	connect(actionDisconnect, SIGNAL(triggered()), this, SLOT(onDisconnect()));
	connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));

	// Channel menu
	connect(actionJoin, SIGNAL(triggered()), this, SLOT(onJoin()));
	connect(actionPart, SIGNAL(triggered()), this, SLOT(onPart()));

	// Help menu
	connect(actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
	connect(actionAboutQt, SIGNAL(triggered()), this, SLOT(onAboutQt()));

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

	if (QSystemTrayIcon::isSystemTrayAvailable())
	{
/*
		QMenu * trayMenu = new QMenu(this);
		QAction *restoreAction = trayMenu->addAction(tr("Restore"));
		connect(restoreAction, SIGNAL(triggered()), this, SLOT(trayActivated()));
		QAction *quitAction = trayMenu->addAction(tr("Quit"));
		connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
*/

		m_trayIcon = new QSystemTrayIcon(QIcon(":/icons/kdamn.svg"), this);
//		m_trayIcon->setContextMenu(trayMenu);
		connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));
		m_trayIcon->show();
	}

	autoConnect();
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
		QString authtoken = dialog.getAuthtoken();
		bool remember = dialog.isRememberPassword();

		QSettings settings(QSettings::IniFormat, QSettings::UserScope, AUTHOR, PRODUCT);
		settings.setValue("login", login);
		settings.setValue("password", remember ? password:"");
		settings.setValue("remember_password", remember);
		settings.setValue("authtoken", authtoken);

		DAmn::getInstance()->setLogin(login);
		DAmn::getInstance()->setToken(authtoken);

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
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, AUTHOR, PRODUCT);
	settings.setValue("login", login);
	settings.setValue("authtoken", authtoken);

	DAmn::getInstance()->setLogin(login);
	DAmn::getInstance()->setToken(authtoken);

	DAmn::getInstance()->connectToServer();
}

void MainWindow::onJoin()
{
	QString channel = QInputDialog::getText(this, tr("Join channel"), tr("Please enter channel to join"), QLineEdit::Normal, "", NULL,  Qt::Dialog | Qt::WindowCloseButtonHint);

	if (!channel.isEmpty())
	{
		DAmn::getInstance()->join(channel);

		QSettings settings(QSettings::IniFormat, QSettings::UserScope, AUTHOR, PRODUCT);
		settings.setValue(QString("channels/%1").arg(channel), 1);
	}
}

void MainWindow::onPart()
{
	ChannelFrame *frame = getCurrentChannelFrame();

	if (frame)
	{
		DAmn::getInstance()->part(frame->getChannel());

		QSettings settings(QSettings::IniFormat, QSettings::UserScope, AUTHOR, PRODUCT);
		settings.setValue(QString("channels/%1").arg(frame->getChannel()), 0);
	}
}

void MainWindow::onConnectServer()
{
	setSystem(tr("Connected to server"));

	QSettings settings(QSettings::IniFormat, QSettings::UserScope, AUTHOR, PRODUCT);

	settings.beginGroup("channels");
	QStringList channels = settings.childKeys();

	foreach(const QString &channel, channels)
	{
		if (settings.value(channel, 0).toInt() == 1) DAmn::getInstance()->join(channel);
	}

	settings.endGroup();
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
}

void MainWindow::onUserPart(const QString &channel, const QString &user, const QString &reason, bool show)
{
	ChannelFrame *frame = getChannelFrame(channel);

	if (frame) frame->userPart(user, reason);
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
}

void MainWindow::onPartChannel(const QString &channel, const QString &reason)
{
	removeChannelFrame(channel);

	QString str = tr("You leaved channel <b>%1</b>").arg(channel);

	if (!reason.isEmpty()) str += QString(" (%1)").arg(reason);

	setSystem(str);
}

void MainWindow::onError(const QString &error)
{
	serverBrowser->append(QString("<div class=\"error\">%1</div>").arg(error));
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

ChannelFrame* MainWindow::getChannelFrame(const QString &channel, bool *focused)
{
	for(int i = 0; i < channelsWidget->count(); ++i)
	{
		if (channelsWidget->tabText(i) == channel)
		{
			if (focused) *focused = channelsWidget->currentIndex() == i;

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

void MainWindow::autoConnect()
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, AUTHOR, PRODUCT);
	QString login = settings.value("login").toString();
	QString token = settings.value("authtoken").toString();

	if (!login.isEmpty() && !token.isEmpty())
	{
		DAmn::getInstance()->setLogin(login);
		DAmn::getInstance()->setToken(token);

		DAmn::getInstance()->connectToServer();
	}
}

void MainWindow::onRequestDAmnToken()
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, AUTHOR, PRODUCT);
	QString login = settings.value("login").toString();
	QString password = settings.value("password").toString();

	// delete previous authtoken because invalid
	settings.setValue("authtoken", "");

	if (settings.value("remember_password").toBool() && !login.isEmpty() && !password.isEmpty())
	{
		OAuth2::getInstance()->login(login, password);
	}
	else
	{
		onConnect();
	}
}
