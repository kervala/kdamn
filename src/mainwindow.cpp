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

	m_damn = new DAmn(this);
	connect(m_damn, SIGNAL(serverConnected()), this, SLOT(onConnectServer()));
	connect(m_damn, SIGNAL(htmlMessageReceived(QString, QString, QString)), this, SLOT(onText(QString, QString, QString)));
	connect(m_damn, SIGNAL(authtokenReceived(QString)), this, SLOT(onReceiveAuthtoken(QString)));
	connect(m_damn, SIGNAL(authenticationRequired()), this, SLOT(onConnect()));

//	connect(m_damn, SIGNAL(imageDownloaded()), this, SLOT(onImage()));
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

void MainWindow::onText(const QString &channel, const QString &user, const QString &text)
{
	outputBrowser->moveCursor(QTextCursor::End);
	outputBrowser->insertHtml("<b>" + user + "</b> " + text + "<br>");
//	outputBrowser->append(user + "> " + text);
}

void MainWindow::onSend()
{
	if (m_damn->send("KervalaTests", inputEdit->text()))
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
		settings.setValue("remember_password", remember);
		settings.setValue("authtoken", authtoken);

		if (remember)
		{
			settings.setValue("password", password);
		}

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
		m_damn->joinChat(channel);

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
		if (settings.value(channel, 0).toInt() == 1) m_damn->joinChat(channel);
	}

	settings.endGroup();
}
