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
#include "configfile.h"

#ifdef USE_QT5

#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

SystrayIcon* SystrayIcon::s_instance = NULL;

SystrayIcon::SystrayIcon(QWidget* parent):QObject(parent), m_status(StatusNormal), m_icon(NULL), m_action(ActionNone),
	m_minimizeAction(NULL), m_restoreAction(NULL), m_quitAction(NULL)
{
	if (!s_instance) s_instance = this;

	update();
}

SystrayIcon::~SystrayIcon()
{
	s_instance = NULL;
}

bool SystrayIcon::create()
{
	if (m_icon) return true;

	QWidget *parentW = qobject_cast<QWidget*>(parent());

	QIcon icon(":/icons/icon.svg");

	// under OS X, icon should be white with dark theme and black with light theme
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	icon.setIsMask(true);
#endif

	m_icon = new QSystemTrayIcon(icon, parentW);
	m_icon->setToolTip(QApplication::applicationName());

	connect(m_icon, SIGNAL(messageClicked()), this, SLOT(onMessageClicked()));

	QMenu *menu = new QMenu(parentW);

	m_minimizeAction = menu->addAction(tr("Minimize"));
	m_restoreAction = menu->addAction(tr("Restore"));
	m_quitAction = menu->addAction(tr("Quit"));

	m_icon->setContextMenu(menu);

	connect(m_minimizeAction, SIGNAL(triggered()), this, SIGNAL(requestMinimize()));
	connect(m_restoreAction, SIGNAL(triggered()), this, SIGNAL(requestRestore()));
	connect(m_quitAction, SIGNAL(triggered()), this, SIGNAL(requestClose()));

	connect(m_icon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(onTrayActivated(QSystemTrayIcon::ActivationReason)));

	updateStatus();

	return true;
}

bool SystrayIcon::release()
{
	if (m_icon)
	{
		m_icon->contextMenu()->deleteLater();
		m_icon->deleteLater();
		m_icon = NULL;
	}

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

	QString iconImage;

	switch(status)
	{
		case StatusTalkOther:
		iconImage = ":/icons/icon_blue.svg";
		break;

		case StatusTalkMe:
		iconImage = ":/icons/icon_red.svg";
		break;

		case StatusNormal:
		default:
		iconImage = ":/icons/icon.svg";
		break;
	}

	QIcon icon(iconImage);

	QApplication::setWindowIcon(icon);

	if (m_icon)
	{
		m_icon->setIcon(icon);
		m_icon->show();
	}
}

SystrayIcon::SystrayStatus SystrayIcon::getStatus(const QString &room) const
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

void SystrayIcon::displayMessage(const QString &message, SystrayAction action)
{
	m_action = action;
	
	if (m_icon) m_icon->showMessage("", message, QSystemTrayIcon::NoIcon);
}

void SystrayIcon::update()
{
	if (!QSystemTrayIcon::isSystemTrayAvailable() || !ConfigFile::getInstance()->getUseSystray())
	{
		release();
	}
	else
	{
		create();
	}
}

void SystrayIcon::onMessageClicked()
{
	if (m_action != ActionNone) emit requestAction(m_action);
}

void SystrayIcon::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) emit requestRestore();
}
