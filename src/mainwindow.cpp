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

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

MainWindow::MainWindow():QMainWindow(), m_damn(NULL), m_trayIcon(NULL)
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

	m_damn = new DAmn(this);
	connect(m_damn, SIGNAL(serverConnected()), this, SLOT(onConnectServer()));
	connect(m_damn, SIGNAL(htmlMessageReceived(QString, QString, QString)), this, SLOT(onText(QString, QString, QString)));
	connect(m_damn, SIGNAL(authtokenReceived(QString, QString)), this, SLOT(onReceiveAuthtoken(QString, QString)));
	connect(m_damn, SIGNAL(authenticationRequired()), this, SLOT(onConnect()));
	connect(m_damn, SIGNAL(topicReceived(QString, QString)), this, SLOT(onTopic(QString, QString)));
	connect(m_damn, SIGNAL(titleReceived(QString, QString)), this, SLOT(onTitle(QString, QString)));
	connect(m_damn, SIGNAL(membersReceived(QString, QList<DAmnMember>)), this, SLOT(onMembers(QString, QList<DAmnMember>)));
	connect(m_damn, SIGNAL(channelJoined(QString)), this, SLOT(onJoinChannel(QString)));
	connect(m_damn, SIGNAL(channelParted(QString, QString)), this, SLOT(onPartChannel(QString, QString)));
	connect(m_damn, SIGNAL(errorReceived(QString)), this, SLOT(onError(QString)));

	if (QSystemTrayIcon::isSystemTrayAvailable())
	{
		QMenu * trayMenu = new QMenu(this);
		QAction *restoreAction = trayMenu->addAction(tr("Restore"));
		connect(restoreAction, SIGNAL(triggered()), this, SLOT(trayActivated()));
		QAction *quitAction = trayMenu->addAction(tr("Quit"));
		connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

		m_trayIcon = new QSystemTrayIcon(QIcon(":/icons/kdamn.svg"), this);
		m_trayIcon->setContextMenu(trayMenu);
		connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));
		m_trayIcon->show();
	}
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
	QMessageBox::about(this, tr("About %1 %2 by %3").arg(PRODUCT).arg(VERSION).arg(AUTHOR), DESCRIPTION);
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
		if (remember) settings.setValue("password", password);
		settings.setValue("remember_password", remember);
		settings.setValue("authtoken", authtoken);

		m_damn->setLogin(login);
		m_damn->setPassword(password);
		m_damn->setAuthtoken(authtoken);
		m_damn->connectToDA();
	}
}

void MainWindow::onDisconnect()
{
	m_damn->disconnect();
}

void MainWindow::onReceiveAuthtoken(const QString &login, const QString &authtoken)
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, AUTHOR, PRODUCT);
	settings.setValue("login", login);
	settings.setValue("authtoken", authtoken);
}

void MainWindow::onJoin()
{
	QString channel = QInputDialog::getText(this, tr("Join channel"), tr("Please enter channel to join"), QLineEdit::Normal, "", NULL,  Qt::Dialog | Qt::WindowCloseButtonHint);

	if (!channel.isEmpty())
	{
		m_damn->join(channel);

		QSettings settings(QSettings::IniFormat, QSettings::UserScope, AUTHOR, PRODUCT);
		settings.setValue(QString("channels/%1").arg(channel), 1);
	}
}

void MainWindow::onPart()
{
}

void MainWindow::onConnectServer()
{
	setSystem(tr("Connected to server"));

	QSettings settings(QSettings::IniFormat, QSettings::UserScope, AUTHOR, PRODUCT);

	settings.beginGroup("channels");
	QStringList channels = settings.childKeys();

	foreach(const QString &channel, channels)
	{
		if (settings.value(channel, 0).toInt() == 1) m_damn->join(channel);
	}

	settings.endGroup();
}

void MainWindow::onText(const QString &channel, const QString &user, const QString &text)
{
	ChannelFrame *frame = getChannelFrame(channel);

	if (frame) frame->setText(user, text);
}

void MainWindow::onTopic(const QString &channel, const QString &topic)
{
	ChannelFrame *frame = getChannelFrame(channel);

	if (frame) frame->setTopic(topic);
}

void MainWindow::onTitle(const QString &channel, const QString &title)
{
	ChannelFrame *frame = getChannelFrame(channel);

	if (frame) frame->setTitle(title);
}

void MainWindow::onMembers(const QString &channel, const QList<DAmnMember> &members)
{
	ChannelFrame *frame = getChannelFrame(channel);

	if (frame) frame->setUsers(members);
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
		if (channelsWidget->tabText(i) == channel)
		{
			channelsWidget->removeTab(i);

			delete channelsWidget->widget(i);
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

void MainWindow::setSystem(const QString &text)
{
	serverBrowser->append(QString("<div class=\"system\">%1</div>").arg(text));
}
