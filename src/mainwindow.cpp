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
#include "mainwindow.h"
#include "moc_mainwindow.cpp"
#include "connectdialog.h"
#include "settingsdialog.h"
#include "joinroomdialog.h"
#include "configfile.h"
#include "roomsdialog.h"
#include "roomframe.h"
#include "oauth2.h"
#include "capturedialog.h"
#include "utils.h"
#include "configfile.h"
#include "updatedialog.h"

#ifdef Q_OS_WIN32
#include <QtWinExtras/QWinTaskbarProgress>
#include <QtWinExtras/QWinTaskbarButton>
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

MainWindow::MainWindow():QMainWindow(), m_manualCheckUpdates(false), m_mustLoginAfterLogout(false)
{
	setupUi(this);

#ifdef Q_OS_WIN32
	m_button = new QWinTaskbarButton(this);
#endif

	// Server menu
	connect(actionConnect, SIGNAL(triggered()), this, SLOT(onConnect()));
	connect(actionDisconnect, SIGNAL(triggered()), this, SLOT(onDisconnect()));
	connect(actionSettings, SIGNAL(triggered()), this, SLOT(onSettings()));
	connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));

	// Room menu
	connect(actionJoin, SIGNAL(triggered()), this, SLOT(onJoin()));
	connect(actionPart, SIGNAL(triggered()), this, SLOT(onPart()));
	connect(actionKnownRooms, SIGNAL(triggered()), this, SLOT(onRooms()));

	// Stash menu
	connect(actionUploadFiles, SIGNAL(triggered()), this, SLOT(onUploadFiles()));
	connect(actionUploadScreenshot, SIGNAL(triggered()), this, SLOT(onUploadScreenshot()));

	// Notes menu
	connect(actionDisplayNotes, SIGNAL(triggered()), this, SLOT(onDisplayNotes()));
	connect(actionSendNote, SIGNAL(triggered()), this, SLOT(onSendNote()));
	connect(actionCheckUrlChanges, SIGNAL(triggered()), this, SLOT(onCheckUrlChanges()));
	connect(actionStopCheckUrlChanges, SIGNAL(triggered()), this, SLOT(onStopCheckUrlChanges()));

	// Help menu
	connect(actionLogs, SIGNAL(triggered()), this, SLOT(onLogs()));
	connect(actionCheckUpdates, SIGNAL(triggered()), this, SLOT(onCheckUpdates()));
	connect(actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
	connect(actionAboutQt, SIGNAL(triggered()), this, SLOT(onAboutQt()));

	QSize size = ConfigFile::getInstance()->getWindowSize();
	if (!size.isNull()) resize(size);

	QPoint pos = ConfigFile::getInstance()->getWindowPosition();
	if (!pos.isNull()) move(pos);

	connect(OAuth2::getInstance(), SIGNAL(loggedOut(bool)), this, SLOT(onLoggedOut(bool)));
	connect(OAuth2::getInstance(), SIGNAL(newVersionDetected(QString, QString, uint, QString)), this, SLOT(onNewVersion(QString, QString, uint, QString)));
	connect(OAuth2::getInstance(), SIGNAL(noNewVersionDetected()), this, SLOT(onNoNewVersion()));
	connect(OAuth2::getInstance(), SIGNAL(uploadProgress(qint64, qint64)), this, SLOT(onProgress(qint64, qint64)));
	connect(OAuth2::getInstance(), SIGNAL(urlChecked(QString, QString)), this, SLOT(onUrlChecked(QString, QString)));

	// Systray
	SystrayIcon *systray = new SystrayIcon(this);
	connect(systray, SIGNAL(requestMinimize()), this, SLOT(onMinimize()));
	connect(systray, SIGNAL(requestRestore()), this, SLOT(onRestore()));
	connect(systray, SIGNAL(requestClose()), this, SLOT(close()));
	connect(systray, SIGNAL(requestAction(SystrayIcon::SystrayAction)), this, SLOT(onSystrayAction(SystrayIcon::SystrayAction)));

	// auto connect to chat server
	autoConnect();

	// check for a new version
	OAuth2::getInstance()->checkUpdates();
}

MainWindow::~MainWindow()
{
}

void MainWindow::showEvent(QShowEvent *e)
{
#ifdef Q_OS_WIN32
	m_button->setWindow(windowHandle());
#endif

	e->accept();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	hide();

	if (OAuth2::getInstance()->isLogged())
	{
		e->ignore();

		m_mustLoginAfterLogout = false;

		OAuth2::getInstance()->logout(false);
	}
	else
	{
		e->accept();
	}
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

void MainWindow::onLogs()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(ConfigFile::getInstance()->getLogsDirectory()));
}

void MainWindow::onCheckUpdates()
{
	m_manualCheckUpdates = true;

	OAuth2::getInstance()->checkUpdates();
}

void MainWindow::onAbout()
{
	QMessageBox::about(this,
		tr("About %1").arg(QApplication::applicationName()),
		QString("%1 %2<br>").arg(QApplication::applicationName()).arg(QApplication::applicationVersion())+
		tr("deviantART Messaging Network (dAmn) chat client")+
		QString("<br><br>")+
		tr("Author: %1").arg("<a href=\"http://kervala.deviantart.com\">Kervala</a><br>")+
		tr("Homepage: %1").arg("<a href=\"http://kervala.deviantart.com/art/kdAmn-387332272\">DeviantArt</a><br>")+
		tr("Support: %1").arg("<a href=\"http://dev.kervala.net/projects/kdamn\">http://dev.kervala.net/projects/kdamn</a>"));
}

void MainWindow::onAboutQt()
{
	QMessageBox::aboutQt(this);
}

void MainWindow::onMinimize()
{
	// only hide window if using systray
	if (isVisible() && ConfigFile::getInstance()->getUseSystray())
	{
		hide();
	}
}

void MainWindow::onRestore()
{
	if (!isVisible())
	{
		if (isMaximized())
		{
			showMaximized();
		}
		else if (isMinimized())
		{
			showNormal();
		}
	}

	raise();
	activateWindow();
}

void MainWindow::onSystrayAction(SystrayIcon::SystrayAction action)
{
	switch(action)
	{
		case SystrayIcon::ActionOpenURL:
		if (!m_urlToCheck.isEmpty()) QDesktopServices::openUrl(m_urlToCheck);
		break;

		case SystrayIcon::ActionReadLastNote:
		QDesktopServices::openUrl(QUrl("https://www.deviantart.com/messages/notes/"));
		break;

		default:
		break;
	}
}

void MainWindow::onConnect()
{
	ConnectDialog dialog(this);

	if (dialog.exec())
	{
		m_mustLoginAfterLogout = true;

		DAmn::getInstance()->disconnect();
		OAuth2::getInstance()->logout(true);
	}
}

void MainWindow::onDisconnect()
{
	DAmn::getInstance()->disconnect();
	OAuth2::getInstance()->logout(false);
}

void MainWindow::onSettings()
{
	SettingsDialog dialog(this);

	if (dialog.exec())
	{
		roomsWidget->updateConfig();
	}
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

void MainWindow::onUploadFiles()
{
	QStringList filenames = QFileDialog::getOpenFileNames(this, tr("Upload filess"));

	QString room;
	
	if (roomsWidget->getCurrentRoomFrame()) room = roomsWidget->getCurrentRoomFrame()->getRoom();

	OAuth2::getInstance()->uploadToStash(filenames, room);
}

void MainWindow::onUploadScreenshot()
{
	WId id = 0;
	QString name;

	{
		CaptureDialog dlg(this);

		if (!dlg.exec()) return;

		id = dlg.getWindowId();
		name = dlg.getWindowName();
	}

	bool minimized = false;

	minimized = RestoreMinimizedWindow(id);

	if (!minimized && !IsUsingComposition())
		PutForegroundWindow(id);

#ifdef USE_QT5
	QPixmap pixmap = QGuiApplication::primaryScreen()->grabWindow(id);
#else
	QPixmap pixmap = QPixmap::grabWindow(id);
#endif

	if (minimized)
	{
		MinimizeWindow(id);
	}
	else
	{
		if (!IsUsingComposition())
		{
			activateWindow();
		}
	}

	int pos = name.indexOf(QRegExp("[\\\\/:? *;<>&-]"));

	if (pos > -1)
	{
		if (pos == 0)
		{
			name = "untitled";
		}
		else
		{
			name = name.mid(0, pos);
		}
	}

	QString filename = QString("%1/%2-%3.png").arg(ConfigFile::getInstance()->getCacheDirectory()).arg(name).arg(QDateTime::currentMSecsSinceEpoch());

	pixmap.save(filename);

	QString room;
	
	if (roomsWidget->getCurrentRoomFrame()) room = roomsWidget->getCurrentRoomFrame()->getRoom();

	QStringList filenames;
	filenames << filename;

	OAuth2::getInstance()->uploadToStash(filenames, room);
}

void MainWindow::onDisplayNotes()
{
	OAuth2::getInstance()->requestNotesDisplayFolder("1", 0);
}

void MainWindow::onSendNote()
{
	OAuth2::getInstance()->prepareNote();
}

void MainWindow::onCheckUrlChanges()
{
	QString url = QInputDialog::getText(this, tr("URL changes check"), tr("Enter URL to check:"), QLineEdit::Normal, m_urlToCheck);

	if (!url.isEmpty())
	{
		m_urlToCheck = url;

		checkUrlChanges();
	}
}

void MainWindow::onStopCheckUrlChanges()
{
	m_urlToCheck.clear();
	m_md5.clear();
}

void MainWindow::autoConnect()
{
	QString login = ConfigFile::getInstance()->getLogin();
	QString password = ConfigFile::getInstance()->getPassword();
	QString damnToken = ConfigFile::getInstance()->getDAmnToken();

	OAuth2::getInstance()->setLogin(login);
	OAuth2::getInstance()->setPassword(password);
	OAuth2::getInstance()->setDAmnToken(damnToken);

	if (!login.isEmpty() && !damnToken.isEmpty())
	{
		DAmn::getInstance()->setLogin(login);
		DAmn::getInstance()->setToken(damnToken);

		DAmn::getInstance()->connectToServer();
	}
	else
	{
		onConnect();
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
	else if (e->type() == QEvent::LanguageChange)
	{
		retranslateUi(this);
	}
	else if (e->type() == QEvent::WindowStateChange)
	{
		if (windowState() & Qt::WindowMinimized)
		{
			QTimer::singleShot(250, this, SLOT(onMinimize()));
		}
	}

	return QMainWindow::event(e);
}

void MainWindow::onLoggedOut(bool reconnect)
{
	if (reconnect)
	{
		if (m_mustLoginAfterLogout)
		{
			m_mustLoginAfterLogout = false;
		}

		roomsWidget->login();
	}
	else
	{
		// only close window if already hidden (when close button pressed)
		if (isHidden()) close();
	}
}

void MainWindow::onNewVersion(const QString &url, const QString &date, uint size, const QString &version)
{
	QMessageBox::StandardButton reply = QMessageBox::question(this,
		tr("New version"),
		tr("Version %1 is available since %2.\n\nDo you want to download it now?").arg(version).arg(date),
		QMessageBox::Yes|QMessageBox::No);

	if (reply != QMessageBox::Yes) return;

	UpdateDialog dialog(this);

	connect(&dialog, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(onProgress(qint64, qint64)));

	dialog.download(url, size);

	if (dialog.exec() == QDialog::Accepted)
	{
		// if user clicked on Install, close kdAmn
		close();
	}
}

void MainWindow::onNoNewVersion()
{
	// silent result if launched at start
	if (!m_manualCheckUpdates) return;

	QMessageBox::information(this,
		tr("No update found"),
		tr("You already have the last %1 version (%2).").arg(QApplication::applicationName()).arg(QApplication::applicationVersion()));

	m_manualCheckUpdates = false;
}

void MainWindow::onProgress(qint64 readBytes, qint64 totalBytes)
{
#ifdef Q_OS_WIN32
	QWinTaskbarProgress *progress = m_button->progress();

	if (readBytes == totalBytes)
	{
		// end
		progress->hide();
	}
	else if (readBytes == 0)
	{
		// beginning
		progress->show();
		progress->setRange(0, totalBytes);
	}
	else
	{
		progress->show();
		progress->setValue(readBytes);
	}
#else
	// TODO: for other OSes
#endif
}

void MainWindow::onUrlChecked(const QString &url, const QString &md5)
{
	if (m_urlToCheck == url)
	{
		if (!m_md5.isEmpty() && md5 != m_md5)
		{
			// changed
			SystrayIcon::getInstance()->setStatus("", SystrayIcon::StatusTalkMe);
			SystrayIcon::getInstance()->displayMessage(tr("Url %1 changed, click here to open it").arg(url), SystrayIcon::ActionOpenURL);
		}

		m_md5 = md5;

		QTimer::singleShot(5000, this, SLOT(checkUrlChanges()));
	}
}

void MainWindow::checkUrlChanges()
{
	if (!m_urlToCheck.isEmpty()) OAuth2::getInstance()->checkUrlChanges(m_urlToCheck);
}
