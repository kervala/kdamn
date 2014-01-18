/*
 *  kdAmn is a deviantART Messaging Network client
 *  Copyright (C) 2013-2014  Cedric OCHS
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
	m_rememberPassword(true), m_method(MethodOAuth2), m_animationFrameDelay(100), m_autosaveDelay(30), m_modified(false),
	m_size(0, 0), m_position(0, 0), m_splitter(0)
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

	if (version == 2) loadVersion2();
	else loadVersion1();

	autosave();

	return true;
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
	m_autosaveDelay = m_settings.value("autosave").toInt();

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

	// window parameters
	m_settings.beginGroup("window");

	m_size = QSize(m_settings.value("width", 0).toInt(), m_settings.value("height", 0).toInt());
	m_position = QPoint(m_settings.value("x", 0).toInt(), m_settings.value("y", 0).toInt());
	m_splitter = m_settings.value("splitter", 0).toInt();

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

	// load cookies
	m_settings.beginGroup("cookies");

	QStringList cookies = m_settings.childKeys();

	m_cookies.clear();

	foreach(const QString &cookie, cookies)
	{
		m_cookies << QNetworkCookie::parseCookies(m_settings.value(cookie).toString().toUtf8());
	}

	m_settings.endGroup();

	return true;
}

bool ConfigFile::save()
{
	// no need to save because no change has been made
	if (!m_modified)
	{
		autosave();
		return true;
	}

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

	// window parameters
	m_settings.beginGroup("window");

	m_settings.setValue("width", m_size.width());
	m_settings.setValue("height", m_size.height());
	m_settings.setValue("x", m_position.x());
	m_settings.setValue("y", m_position.y());
	m_settings.setValue("splitter", m_splitter);

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

	// save cookies
	m_settings.beginGroup("cookies");

	for(int i = 0; i < m_cookies.size(); ++i)
	{
		m_settings.setValue(QString::number(i), QString::fromUtf8(m_cookies.at(i).toRawForm()));
	}

	m_settings.endGroup();

	modified(false);

	autosave();

	return true;
}

QString ConfigFile::getLogin() const
{
	return m_login;
}

void ConfigFile::setLogin(const QString &login)
{
	if (m_login == login) return;

	m_login = login;
	modified(true);
}

QString ConfigFile::getPassword() const
{
	return m_password;
}

void ConfigFile::setPassword(const QString &password)
{
	if (m_password == password) return;

	m_password = password;
	modified(true);
}

bool ConfigFile::isRememberPassword() const
{
	return m_rememberPassword;
}

void ConfigFile::rememberPassword(bool remember)
{
	if (m_rememberPassword == remember) return;

	m_rememberPassword = remember;
	modified(true);
}

QString ConfigFile::getDAmnToken() const
{
	return m_damnToken;
}

void ConfigFile::setDAmnToken(const QString &token)
{
	if (m_damnToken == token) return;

	m_damnToken = token;
	modified(true);
}

QString ConfigFile::getAccessToken() const
{
	return m_accessToken;
}

void ConfigFile::setAccessToken(const QString &token)
{
	if (m_accessToken == token) return;

	m_accessToken = token;
	modified(true);
}

QString ConfigFile::getRefreshToken() const
{
	return m_refreshToken;
}

void ConfigFile::setRefreshToken(const QString &token)
{
	if (m_refreshToken == token) return;

	m_refreshToken = token;
	modified(true);
}

DAmnTokenMethod ConfigFile::getDAmnTokenMethod() const
{
	return m_method;
}

void ConfigFile::setDAmnTokenMethod(DAmnTokenMethod method)
{
	if (m_method == method) return;

	m_method = method;
	modified(true);
}

QSize ConfigFile::getWindowSize() const
{
	return m_size;
}

void ConfigFile::setWindowSize(const QSize &size)
{
	if (m_size == size || size.width() < 10 || size.height() < 10) return;

	m_size = size;
	modified(true);
}

QPoint ConfigFile::getWindowPosition() const
{
	return m_position;
}

void ConfigFile::setWindowPosition(const QPoint &pos)
{
	if (m_position == pos || pos.isNull()) return;

	m_position = pos;
	modified(true);
}

int ConfigFile::getAnimationFrameDelay() const
{
	return m_animationFrameDelay;
}

void ConfigFile::setAnimationFrameDelay(int delay)
{
	if (m_animationFrameDelay == delay) return;

	m_animationFrameDelay = delay;
	modified(true);
}

int ConfigFile::getAutosaveDelay() const
{
	return m_autosaveDelay;
}

void ConfigFile::setAutosaveDelay(int delay)
{
	if (m_autosaveDelay == delay) return;

	m_autosaveDelay = delay;
	modified(true);
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

		modified(true);
	}

	return it;
}

void ConfigFile::setRoomAutoConnect(const QString &name, bool autoconnect)
{
	ConfigRoomsIterator it = getRoom(name, true);

	if (it->autoconnect == autoconnect) return;

	it->autoconnect = autoconnect;
	it->updateToValue();

	modified(true);
}

void ConfigFile::setRoomConnected(const QString &name, bool connected)
{
	ConfigRoomsIterator it = getRoom(name, true);

	if (it->connected == connected) return;

	it->connected = connected;
	it->updateToValue();

	modified(true);
}

void ConfigFile::setRoomFocused(const QString &name, bool focused)
{
	ConfigRoomsIterator it = getRoom(name, true);

	if (it->focused == focused) return;

	it->focused = focused;
	it->updateToValue();

	modified(true);
}

void ConfigFile::setRoomOrder(const QString &name, int order)
{
	ConfigRoomsIterator it = getRoom(name, true);

	if (it->order == order) return;

	it->order = order;
	it->updateToValue();

	modified(true);
}

void ConfigFile::setRoomValue(const QString &name, int value)
{
	ConfigRoomsIterator it = getRoom(name, true);

	if (it->value == value) return;

	it->value = value;
	it->updateFromValue();

	modified(true);
}

void ConfigFile::autosave()
{
	if (m_autosaveDelay > 0) QTimer::singleShot(m_autosaveDelay * 60 * 1000, this, SLOT(save()));
}

void ConfigFile::modified(bool modified)
{
	m_modified = modified;
}

QList<QNetworkCookie> ConfigFile::getCookies() const
{
	return m_cookies;
}

void ConfigFile::setCookies(const QList<QNetworkCookie> &cookies)
{
	if (m_cookies == cookies) return;

	m_cookies = cookies;

	modified(true);
}
