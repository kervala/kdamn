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

#ifndef CONFIGFILE_H
#define CONFIGFILE_H

struct ConfigChannel
{
	QString name;
	bool autoconnect;
	bool connected;
	bool focused;
	int order;

	int value;

	ConfigChannel()
	{
		autoconnect = false;
		connected = false;
		focused = false;
		order = 0;

		value = 0;
	}

	void updateToValue()
	{
	}

	void updateFromValue()
	{
		autoconnect = (value & 1) == 1;
		connected = ((value >> 1) & 1) == 1;
		focused = ((value >> 2) & 1) == 1;
		order = value >> 8;
	}
};

typedef QList<ConfigChannel> ConfigChannels;
typedef ConfigChannels::iterator ConfigChannelsIterator;

class ConfigFile : public QObject
{
	Q_OBJECT

public:
	ConfigFile(QObject* parent);
	virtual ~ConfigFile();

	static ConfigFile* getInstance() { return s_instance; }

	bool load();
	bool save();

	QString getLogin() const;
	void setLogin(const QString &login);

	QString getPassword() const;
	void setPassword(const QString &password);

	bool isRememberPassword() const;
	void rememberPassword(bool remember);

	QString getDAmnToken() const;
	void setDAmnToken(const QString &token);

	ConfigChannels getChannels() const;
	ConfigChannelsIterator getChannel(const QString &name, bool insert = false);
	void setChannelValue(const QString &channel, int value);
	void setChannelAutoConnect(const QString &channel, bool autoconnect);
	void setChannelConnected(const QString &channel, bool connected);
	void setChannelFocused(const QString &channel, bool focused);
	void setChannelOrder(const QString &channel, int order);

private:
	static ConfigFile* s_instance;

	QSettings m_settings;

	QString m_login;
	QString m_password;
	bool m_rememberPassword;
	QString m_damnToken;
	ConfigChannels m_channels;
};

#endif
