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

ConfigFile::ConfigFile(QObject* parent):QObject(parent), m_settings(QSettings::IniFormat, QSettings::UserScope, AUTHOR, PRODUCT)
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
	// connection parameters
	m_login = m_settings.value("login").toString();
	m_password = m_settings.value("password").toString();
	m_rememberPassword = m_settings.value("remember_password").toBool();
	m_damnToken = m_settings.value("authtoken").toString();

	// load channels
	m_settings.beginGroup("channels");

	QStringList channels = m_settings.childKeys();

	m_channels.clear();

	foreach(const QString &channel, channels)
	{
		setChannelValue(channel, m_settings.value(channel, 0).toInt());
	}

	m_settings.endGroup();

	return true;
}

bool ConfigFile::save()
{
	// connection parameters
	m_settings.setValue("login", m_login);
	m_settings.setValue("password", m_password);
	m_settings.setValue("remember_password", m_rememberPassword);
	m_settings.setValue("authtoken", m_damnToken);

	// save channels
	ConfigChannelsIterator it = m_channels.begin();

	while(it != m_channels.end())
	{
		m_settings.setValue(QString("channels/%1").arg(it->name), it->value);

		++it;
	}

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

ConfigChannels ConfigFile::getChannels() const
{
	return m_channels;
}

ConfigChannelsIterator ConfigFile::getChannel(const QString &name, bool insert)
{
	ConfigChannelsIterator it = m_channels.begin();

	while(it != m_channels.end())
	{
		if (it->name.toLower() == name.toLower()) break;

		++it;
	}

	if (it == m_channels.end())
	{
		ConfigChannel channel;
		channel.name = name;

		it = m_channels.insert(it, channel);
	}

	return it;
}

void ConfigFile::setChannelAutoConnect(const QString &name, bool autoconnect)
{
	ConfigChannelsIterator it = getChannel(name, true);

	it->autoconnect = autoconnect;
	it->updateToValue();
}

void ConfigFile::setChannelConnected(const QString &name, bool connected)
{
	ConfigChannelsIterator it = getChannel(name, true);

	it->connected = connected;
	it->updateToValue();
}

void ConfigFile::setChannelFocused(const QString &name, bool focused)
{
	ConfigChannelsIterator it = getChannel(name, true);

	it->focused = focused;
	it->updateToValue();
}

void ConfigFile::setChannelOrder(const QString &name, int order)
{
	ConfigChannelsIterator it = getChannel(name, true);

	it->order = order;
	it->updateToValue();
}

void ConfigFile::setChannelValue(const QString &name, int value)
{
	ConfigChannelsIterator it = getChannel(name, true);

	it->value = value;
	it->updateFromValue();
}
