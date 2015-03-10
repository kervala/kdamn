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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include "systrayicon.h"

class QWinTaskbarButton;

class MainWindow : public QMainWindow, public Ui::MainWindow
{
	Q_OBJECT

public:
	MainWindow();
	virtual ~MainWindow();

	void autoConnect();

public slots:
	// server menu
	void onConnect();
	void onDisconnect();
	void onSettings();

	// room menu
	void onJoin();
	void onPart();
	void onRooms();

	// stash menu
	void onUploadFiles();
	void onUploadScreenshot();

	// notes menu
	void onDisplayNotes();
	void onSendNote();
	void onCheckUrlChanges();
	void onStopCheckUrlChanges();

	// help menu
	void onLogs();
	void onCheckUpdates();
	void onAbout();
	void onAboutQt();

	// other
	void onMinimize();
	void onRestore();
	void onSystrayAction(SystrayIcon::SystrayAction action);
	void checkUrlChanges();

	// signals from OAuth2
	void onLoggedOut(bool reconnect);
	void onNewVersion(const QString &url, const QString &date, uint size, const QString &version);
	void onNoNewVersion();
	void onProgress(qint64 readBytes, qint64 totalBytes);
	void onUrlChecked(const QString &url, const QString &md5);

protected:
	void showEvent(QShowEvent *e);
	void closeEvent(QCloseEvent *e);
	void resizeEvent(QResizeEvent *e);
	void moveEvent(QMoveEvent *e);
	bool event(QEvent *e);

	QWinTaskbarButton *m_button;
	bool m_manualCheckUpdates;
	bool m_mustLoginAfterLogout;

	QString m_urlToCheck;
	QString m_md5;
};

#endif
