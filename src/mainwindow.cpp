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
#include "joinroomdialog.h"
#include "configfile.h"
#include "roomsdialog.h"
#include "roomframe.h"
#include "systrayicon.h"
#include "oauth2.h"

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

MainWindow::MainWindow():QMainWindow()
{
	setupUi(this);

	OAuth2::setMainWindowId(winId());

	// Server menu
	connect(actionConnect, SIGNAL(triggered()), this, SLOT(onConnect()));
	connect(actionDisconnect, SIGNAL(triggered()), this, SLOT(onDisconnect()));
	connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));

	// Room menu
	connect(actionJoin, SIGNAL(triggered()), this, SLOT(onJoin()));
	connect(actionPart, SIGNAL(triggered()), this, SLOT(onPart()));
	connect(actionKnownRooms, SIGNAL(triggered()), this, SLOT(onRooms()));

	// Help menu
	connect(actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
	connect(actionAboutQt, SIGNAL(triggered()), this, SLOT(onAboutQt()));

	new ConfigFile(this);

	new SystrayIcon(this);

	QSize size = ConfigFile::getInstance()->getWindowSize();
	if (!size.isNull()) resize(size);

	QPoint pos = ConfigFile::getInstance()->getWindowPosition();
	if (!pos.isNull()) move(pos);

	autoConnect();
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	hide();

	e->accept();
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
	ConfigFile::getInstance()->setWindowSize(e->size());

	e->accept();
}

void MainWindow::moveEvent(QMoveEvent *e)
{
	ConfigFile::getInstance()->setWindowPosition(QPoint(x(), y()));

	e->accept();
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
		roomsWidget->login();
	}
}

void MainWindow::onDisconnect()
{
	DAmn::getInstance()->disconnect();
}

void MainWindow::onJoin()
{
	JoinRoomDialog dialog(this);

	if (dialog.exec())
	{
		QString room = dialog.getRoom();

		DAmn::getInstance()->join(room);

		ConfigFile::getInstance()->setRoomAutoConnect(room, true);
	}
}

void MainWindow::onPart()
{
	RoomFrame *frame = roomsWidget->getCurrentRoomFrame();

	if (frame)
	{
		DAmn::getInstance()->part(frame->getRoom());

		ConfigFile::getInstance()->setRoomAutoConnect(frame->getRoom(), false);
	}
}

void MainWindow::onRooms()
{
	RoomsDialog dialog(this);

	if (dialog.exec())
	{
	}
}

void MainWindow::trayActivated(QSystemTrayIcon::ActivationReason reseaon)
{
}

void MainWindow::autoConnect()
{
	QString login = ConfigFile::getInstance()->getLogin();
	QString password = ConfigFile::getInstance()->getPassword();
	QString damnToken = ConfigFile::getInstance()->getDAmnToken();
	QString accessToken = ConfigFile::getInstance()->getAccessToken();
	QString refreshToken = ConfigFile::getInstance()->getRefreshToken();

	OAuth2::getInstance()->setLogin(login);
	OAuth2::getInstance()->setPassword(password);
	OAuth2::getInstance()->setDAmnToken(damnToken);
	OAuth2::getInstance()->setAccessToken(accessToken, refreshToken);

	if (!login.isEmpty() && !damnToken.isEmpty())
	{
		DAmn::getInstance()->setLogin(login);
		DAmn::getInstance()->setToken(damnToken);

		DAmn::getInstance()->connectToServer();
	}
}

bool MainWindow::event(QEvent *e)
{
	if (e->type() == QEvent::WindowActivate)
	{
		roomsWidget->onRoomFocus(roomsWidget->currentIndex());
	}
	else if (e->type() == QEvent::WindowDeactivate)
	{
		roomsWidget->onRoomFocus(-1);
	}

	return QMainWindow::event(e);
}
