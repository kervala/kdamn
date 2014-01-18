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

#ifndef CONFIGFILE_H
#define CONFIGFILE_H

struct ConfigRoom
{
	QString name;
	bool autoconnect;
	bool connected;
	bool focused;
	int order;

	int value;

	ConfigRoom()
	{
		autoconnect = false;
		connected = false;
		focused = false;
		order = 0;

		value = 0;
	}

	void updateToValue()
	{
		value = 0;

		value |= autoconnect ? 1:0;
		value |= connected ? 2:0;
		value |= focused ? 4:0;
		order |= value << 8;
	}

	void updateFromValue()
	{
		autoconnect = (value & 1) == 1;
		connected = ((value >> 1) & 1) == 1;
		focused = ((value >> 2) & 1) == 1;
		order = value >> 8;
	}
};

enum DAmnTokenMethod
{
	MethodOAuth2,
	MethodSite
};

typedef QList<ConfigRoom> ConfigRooms;
typedef ConfigRooms::iterator ConfigRoomsIterator;

class ConfigFile : public QObject
{
	Q_OBJECT

public:
	ConfigFile(QObject* parent);
	virtual ~ConfigFile();

	static ConfigFile* getInstance() { return s_instance; }

	QString getLogin() const;
	void setLogin(const QString &login);

	QString getPassword() const;
	void setPassword(const QString &password);

	bool isRememberPassword() const;
	void rememberPassword(bool remember);

	QString getDAmnToken() const;
	void setDAmnToken(const QString &token);

	QString getAccessToken() const;
	void setAccessToken(const QString &token);

	QString getRefreshToken() const;
	void setRefreshToken(const QString &token);

	DAmnTokenMethod getDAmnTokenMethod() const;
	void setDAmnTokenMethod(DAmnTokenMethod method);

	QSize getWindowSize() const;
	void setWindowSize(const QSize &size);

	QPoint getWindowPosition() const;
	void setWindowPosition(const QPoint &pos);

	int getAnimationFrameDelay() const;
	void setAnimationFrameDelay(int delay);

	int getAutoSaveDelay() const;
	void setAutoSaveDelay(int delay);

	ConfigRooms getRooms() const;
	ConfigRoomsIterator getRoom(const QString &name, bool insert = false);
	void setRoomValue(const QString &room, int value);
	void setRoomAutoConnect(const QString &room, bool autoconnect);
	void setRoomConnected(const QString &room, bool connected);
	void setRoomFocused(const QString &room, bool focused);
	void setRoomOrder(const QString &room, int order);

	QList<QNetworkCookie> getCookies() const;
	void setCookies(const QList<QNetworkCookie> &cookies);

	bool getDisplayTimestamps() const;
	void setDisplayTimestamps(bool display);

	bool getEnableAnimations() const;
	void setEnableAnimations(bool enable);

public slots:
	bool load();
	bool save();

private:
	void autoSave();
	void modified(bool modified);

	static ConfigFile* s_instance;

	bool loadVersion1();
	bool loadVersion2();

	QSettings m_settings;

	QString m_login;
	QString m_password;
	bool m_rememberPassword;
	QString m_damnToken;
	DAmnTokenMethod m_method;
	ConfigRooms m_rooms;
	QString m_accessToken;
	QString m_refreshToken;
	int m_animationFrameDelay;
	bool m_displayTimestamps;
	bool m_enableAnimations;
	int m_autoSaveDelay;
	bool m_modified;

	QSize m_size;
	QPoint m_position;
	int m_splitter;

	QList<QNetworkCookie> m_cookies;
};

#endif
