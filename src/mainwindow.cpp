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
#include "damn.h"

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
	connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));

	// Channel menu
	connect(actionJoin, SIGNAL(triggered()), this, SLOT(onJoin()));

	// Help menu
	connect(actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
	connect(actionAboutQt, SIGNAL(triggered()), this, SLOT(onAboutQt()));

	connect(inputEdit, SIGNAL(returnPressed()), this, SLOT(onSend()));

	connect(outputBrowser, SIGNAL(anchorClicked(QUrl)), this, SLOT(onUrl(QUrl)));

	m_usersModel = new QStringListModel(this);
	usersView->setModel(m_usersModel);

	outputBrowser->document()->setDefaultStyleSheet(".timestamp { color: #999; }\n.username { font-weight: bold; }\n");

	m_damn = new DAmn(this);
	connect(m_damn, SIGNAL(serverConnected()), this, SLOT(onConnectServer()));
	connect(m_damn, SIGNAL(htmlMessageReceived(QString, QString, QString)), this, SLOT(onText(QString, QString, QString)));
	connect(m_damn, SIGNAL(authtokenReceived(QString)), this, SLOT(onReceiveAuthtoken(QString)));
	connect(m_damn, SIGNAL(authenticationRequired()), this, SLOT(onConnect()));
	connect(m_damn, SIGNAL(topicReceived(QString, QString)), this, SLOT(onTopic(QString, QString)));
	connect(m_damn, SIGNAL(titleReceived(QString, QString)), this, SLOT(onTitle(QString, QString)));
	connect(m_damn, SIGNAL(membersReceived(QString, QList<DAmnMember>)), this, SLOT(onMembers(QString, QList<DAmnMember>)));
	connect(m_damn, SIGNAL(channelJoined(QString)), this, SLOT(onJoinChannel(QString)));
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

void MainWindow::onSend()
{
	if (!m_channel.isEmpty() && m_damn->send(m_channel, inputEdit->text()))
	{
		inputEdit->clear();
	}
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

void MainWindow::onImage(const QString &md5)
{
}

void MainWindow::onUrl(const QUrl &url)
{
	QDesktopServices::openUrl(url);
}

void MainWindow::onReceiveAuthtoken(const QString &authtoken)
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, AUTHOR, PRODUCT);
	settings.setValue("authtoken", authtoken);
}

void MainWindow::onJoin()
{
	QString channel = QInputDialog::getText(this, tr("Join channel"), tr("Please enter channel to join"));

	if (!channel.isEmpty())
	{
		m_damn->join(channel);

		QSettings settings(QSettings::IniFormat, QSettings::UserScope, AUTHOR, PRODUCT);
		settings.setValue(QString("channels/%1").arg(channel), 1);
	}
}

void MainWindow::onConnectServer()
{
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
	QString timestamp = QTime::currentTime().toString();

	outputBrowser->append(QString("<div class=\"normal\"><span class=\"timestamp\">%1</span> <span class=\"username\">&lt;%2&gt;</span> %3</div>").arg(timestamp).arg(user).arg(text));
}

void MainWindow::onTopic(const QString &channel, const QString &topic)
{
	outputBrowser->append(tr("<div class=\"title\">Topic is %1</div>").arg(topic));
}

void MainWindow::onTitle(const QString &channel, const QString &title)
{
	outputBrowser->append(tr("<div class=\"title\">Title is %1</div>").arg(title));
}

void MainWindow::onMembers(const QString &channel, const QList<DAmnMember> &members)
{
	QStringList list;
	int min = 65536;
	int max = 0;

	foreach(const DAmnMember &member, members)
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

void MainWindow::onJoinChannel(const QString &channel)
{
	m_channel = channel;

	// TODO: tabbar there
}
