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
#include "systrayicon.h"

#ifdef USE_QT5

#ifdef UNITY_HACK
#undef signals

extern "C"
{
	#include <libappindicator/app-indicator.h>
}

#define signals public
#endif

#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

SystrayIcon* SystrayIcon::s_instance = NULL;

SystrayIcon::SystrayIcon(QObject* parent):QObject(parent), m_status(StatusNormal), m_icon(NULL)
{
	if (!s_instance) s_instance = this;

	create();
}

SystrayIcon::~SystrayIcon()
{
	s_instance = NULL;
}

bool SystrayIcon::create()
{
	if (!QSystemTrayIcon::isSystemTrayAvailable()) return false;

/*
	Unity hack:

	gsettings get com.canonical.Unity.Panel systray-whitelist // will print allowed applications in JSON list
	gsettings set com.canonical.Unity.Panel systray-whitelist <whitelist> // set whitelist

	Example if current list ["Dropbox", "Foo"] – and we want to add "Bar" application:

	gsettings set com.canonical.Unity.Panel systray-whitelist "['Dropbox', 'Foo', 'Bar']"
*/

	m_icon = new QSystemTrayIcon(QIcon(":/icons/icon.svg"), this);

	connect(m_icon, SIGNAL(messageClicked()), this, SLOT(onMessageClicked()));

//	QMenu *trayMenu = new QMenu(m_trayIcon);
//	QAction *restoreAction = trayMenu->addAction(tr("Restore"));
//	connect(restoreAction, SIGNAL(triggered()), this, SLOT(trayActivated()));
//	QAction *quitAction = trayMenu->addAction(tr("Quit"));
//	connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

//	m_trayIcon->setContextMenu(trayMenu);
//	connect(m_icon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));
	m_icon->show();

	return true;
}

void SystrayIcon::updateStatus()
{
	SystrayStatus status = StatusNormal;

	SystrayStatusesIterator it = m_rooms.begin();

	while(it != m_rooms.end())
	{
		if (it.value() > status) status = it.value();

		++it;
	}

	QString icon;

	switch(status)
	{
		case StatusNormal: icon = ":/icons/icon.svg"; break;
		case StatusTalkOther: icon = ":/icons/icon_blue.svg"; break;
		case StatusTalkMe: icon = ":/icons/icon_red.svg"; break;
		default: break;
	}

	if (m_icon) m_icon->setIcon(QIcon(icon));
}

SystrayStatus SystrayIcon::getStatus(const QString &room) const
{
	SystrayStatusesConstIterator it = m_rooms.find(room);

	if (it == m_rooms.end()) return StatusUndefined;

	return it.value();
}

void SystrayIcon::setStatus(const QString &room, SystrayStatus status)
{
	m_rooms[room] = status;

	updateStatus();
}

void SystrayIcon::displayMessage(const QString &message, const QString &url)
{
	m_url = url;
	m_icon->showMessage("", message, QSystemTrayIcon::NoIcon);
}

void SystrayIcon::onMessageClicked()
{
	if (!m_url.isEmpty()) QDesktopServices::openUrl(m_url);
}
