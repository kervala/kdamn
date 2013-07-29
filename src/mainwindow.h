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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include "damn.h"

class ChannelFrame;

class MainWindow : public QMainWindow, public Ui::MainWindow
{
	Q_OBJECT

public:
	MainWindow();
	virtual ~MainWindow();

	void autoConnect();
	bool createChannelFrame(const QString &channel);
	bool removeChannelFrame(const QString &channel);
	ChannelFrame* getChannelFrame(const QString &channel);
	ChannelFrame* getCurrentChannelFrame();

	void setSystem(const QString &text);

public slots:
	// server menu
	void onConnect();
	void onDisconnect();

	// channel menu
	void onJoin();
	void onPart();
	void onChannels();

	// help menu
	void onAbout();
	void onAboutQt();

	// slots activated from DAmn signals
	void onRequestDAmnToken();
	void onText(const QString &channel, const QString &user, MessageType type, const QString &text, bool html);
	void onReceiveAuthtoken(const QString &login, const QString &authtoken);
	void onConnectServer();
	void onUsers(const QString &channel, const QStringList &users);
	void onJoinChannel(const QString &channel);
	void onPartChannel(const QString &channel, const QString &reason);
	void onUserJoin(const QString &channel, const QString &user, bool show);
	void onUserPart(const QString &channel, const QString &user, const QString &reason, bool show);
	void onUserPriv(const QString &channel, const QString &user, const QString &by, const QString &pc);
	void onError(const QString &error);

	// other
	void trayActivated(QSystemTrayIcon::ActivationReason reseaon);
	void onChannelFocus(int index);
	void onFocus(QWindow *window);

protected:
	void closeEvent(QCloseEvent *event);
	void updateSystrayIcon(const QString &channel, const QString &user, const QString &html);
};

#endif
