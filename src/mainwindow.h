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

class DAmn;
struct DAmnMember;
class ChannelFrame;

class MainWindow : public QMainWindow, public Ui::MainWindow
{
	Q_OBJECT

public:
	MainWindow();
	virtual ~MainWindow();

	ChannelFrame* getChannelFrame(const QString &channel, bool *focused = NULL);

	void setSystem(const QString &text);

public slots:
	// server menu
	void onConnect();

	// channel menu
	void onJoin();

	// help menu
	void onAbout();
	void onAboutQt();

	// slots activated from DAmn signals
	void onText(const QString &channel, const QString &user, const QString &text);
	void onImage(const QString &md5);
	void onReceiveAuthtoken(const QString &login, const QString &authtoken);
	void onConnectServer();
	void onTopic(const QString &channel, const QString &topic);
	void onTitle(const QString &channel, const QString &title);
	void onMembers(const QString &channel, const QList<DAmnMember> &members);
	void onJoinChannel(const QString &channel);
	void onError(const QString &error);

protected:
	void closeEvent(QCloseEvent *event);

private:
	DAmn *m_damn;
	QSystemTrayIcon *m_trayIcon;
};

#endif
