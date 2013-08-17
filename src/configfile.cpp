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
#include "configfile.h"

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

ConfigFile* ConfigFile::s_instance = NULL;

ConfigFile::ConfigFile(QObject* parent):QObject(parent), m_settings(QSettings::IniFormat, QSettings::UserScope, AUTHOR, PRODUCT),
	m_rememberPassword(true), m_method(MethodOAuth2), m_animationFrameDelay(100)
{
	if (!s_instance) s_instance = this;

	load();
}

ConfigFile::~ConfigFile()
{
	save();

	s_instance = NULL;
}

bool ConfigFile::load()
{
	int version = m_settings.value("version", 1).toInt();

	if (version == 1) return loadVersion1();
	if (version == 2) return loadVersion2();

	return false;
}

bool ConfigFile::loadVersion1()
{
	// server parameters
	m_login = m_settings.value("login").toString();
	m_password = m_settings.value("password").toString();
	m_rememberPassword = m_settings.value("remember_password", true).toBool();
	m_damnToken = m_settings.value("authtoken").toString();
	m_method = MethodOAuth2;

	// load rooms
	m_settings.beginGroup("channels");

	QStringList rooms = m_settings.childKeys();

	m_rooms.clear();

	foreach(const QString &room, rooms)
	{
		setRoomValue(room, m_settings.value(room, 0).toInt());
	}

	m_settings.endGroup();

	return true;
}

bool ConfigFile::loadVersion2()
{
	// server parameters
	m_settings.beginGroup("server");

	m_login = m_settings.value("login").toString();
	m_password = m_settings.value("password").toString();
	m_rememberPassword = m_settings.value("remember_password", true).toBool();
	m_damnToken = m_settings.value("damntoken").toString();
	m_accessToken = m_settings.value("accesstoken").toString();
	m_refreshToken = m_settings.value("refreshtoken").toString();
	m_method = m_settings.value("damntoken_method", "oauth2").toString() == "oauth2" ? MethodOAuth2:MethodSite;

	m_settings.endGroup();

	// chat parameters
	m_settings.beginGroup("chat");

	m_animationFrameDelay = m_settings.value("animation_frame_delay", 100).toInt();

	m_settings.endGroup();

	// load rooms
	m_settings.beginGroup("rooms");

	QStringList rooms = m_settings.childKeys();

	m_rooms.clear();

	foreach(const QString &room, rooms)
	{
		setRoomValue(room, m_settings.value(room, 0).toInt());
	}

	m_settings.endGroup();

	return true;
}

bool ConfigFile::save()
{
	// clear previous entries
	m_settings.clear();

	// general parameters
	m_settings.setValue("version", 2);

	// server parameters
	m_settings.beginGroup("server");

	m_settings.setValue("login", m_login);
	m_settings.setValue("password", m_password);
	m_settings.setValue("remember_password", m_rememberPassword);
	m_settings.setValue("damntoken", m_damnToken);
	m_settings.setValue("accesstoken", m_accessToken);
	m_settings.setValue("refreshtoken", m_refreshToken);

	m_settings.endGroup();

	// chat parameters
	m_settings.beginGroup("chat");

	m_settings.setValue("animation_frame_delay", m_animationFrameDelay);

	m_settings.endGroup();

	// save rooms
	m_settings.beginGroup("rooms");

	ConfigRoomsIterator it = m_rooms.begin();

	while(it != m_rooms.end())
	{
		m_settings.setValue(it->name, it->value);

		++it;
	}

	m_settings.endGroup();

	return true;
}

QString ConfigFile::getLogin() const
{
	return m_login;
}

void ConfigFile::setLogin(const QString &login)
{
	m_login = login;
}

QString ConfigFile::getPassword() const
{
	return m_password;
}

void ConfigFile::setPassword(const QString &password)
{
	m_password = password;
}

bool ConfigFile::isRememberPassword() const
{
	return m_rememberPassword;
}

void ConfigFile::rememberPassword(bool remember)
{
	m_rememberPassword = remember;
}

QString ConfigFile::getDAmnToken() const
{
	return m_damnToken;
}

void ConfigFile::setDAmnToken(const QString &token)
{
	m_damnToken = token;
}

QString ConfigFile::getAccessToken() const
{
	return m_accessToken;
}

void ConfigFile::setAccessToken(const QString &token)
{
	m_accessToken = token;
}

QString ConfigFile::getRefreshToken() const
{
	return m_refreshToken;
}

void ConfigFile::setRefreshToken(const QString &token)
{
	m_refreshToken = token;
}

DAmnTokenMethod ConfigFile::getDAmnTokenMethod() const
{
	return m_method;
}

void ConfigFile::setDAmnTokenMethod(DAmnTokenMethod method)
{
	m_method = method;
}

int ConfigFile::getAnimationFrameDelay() const
{
	return m_animationFrameDelay;
}

void ConfigFile::setAnimationFrameDelay(int delay)
{
	m_animationFrameDelay = delay;
}

ConfigRooms ConfigFile::getRooms() const
{
	return m_rooms;
}

ConfigRoomsIterator ConfigFile::getRoom(const QString &name, bool insert)
{
	ConfigRoomsIterator it = m_rooms.begin();

	while(it != m_rooms.end())
	{
		if (it->name.toLower() == name.toLower()) break;

		++it;
	}

	if (it == m_rooms.end())
	{
		ConfigRoom room;
		room.name = name;

		it = m_rooms.insert(it, room);
	}

	return it;
}

void ConfigFile::setRoomAutoConnect(const QString &name, bool autoconnect)
{
	ConfigRoomsIterator it = getRoom(name, true);

	it->autoconnect = autoconnect;
	it->updateToValue();
}

void ConfigFile::setRoomConnected(const QString &name, bool connected)
{
	ConfigRoomsIterator it = getRoom(name, true);

	it->connected = connected;
	it->updateToValue();
}

void ConfigFile::setRoomFocused(const QString &name, bool focused)
{
	ConfigRoomsIterator it = getRoom(name, true);

	it->focused = focused;
	it->updateToValue();
}

void ConfigFile::setRoomOrder(const QString &name, int order)
{
	ConfigRoomsIterator it = getRoom(name, true);

	it->order = order;
	it->updateToValue();
}

void ConfigFile::setRoomValue(const QString &name, int value)
{
	ConfigRoomsIterator it = getRoom(name, true);

	it->value = value;
	it->updateFromValue();
}
