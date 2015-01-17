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

#define DECLARE_SIMPLE_VAR(type, function, var) \
public:\
	void set##function(type var);\
	type get##function() const;\
protected:\
	type m_##var;

#define DECLARE_TYPED_VAR(type, function, var) \
public:\
	void set##function(const type &var);\
	type get##function() const;\
protected:\
	type m_##var;

#define DECLARE_QSTRING_VAR(function, var) \
DECLARE_TYPED_VAR(QString, function, var);

#define DECLARE_INT_VAR(function, var) \
DECLARE_SIMPLE_VAR(int, function, var);

#define DECLARE_BOOL_VAR(function, var) \
DECLARE_SIMPLE_VAR(bool, function, var);

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

	ConfigRooms getRooms() const;
	ConfigRoomsIterator getRoom(const QString &name, bool insert = false);
	void setRoomValue(const QString &room, int value);
	void setRoomAutoConnect(const QString &room, bool autoconnect);
	void setRoomConnected(const QString &room, bool connected);
	void setRoomFocused(const QString &room, bool focused);
	void setRoomOrder(const QString &room, int order);

	QList<QNetworkCookie> getCookies() const;
	void setCookies(const QList<QNetworkCookie> &cookies);

DECLARE_QSTRING_VAR(Login, login);
DECLARE_QSTRING_VAR(Password, password);
DECLARE_BOOL_VAR(RememberPassword, rememberPassword);
DECLARE_QSTRING_VAR(DAmnToken, damnToken);
DECLARE_QSTRING_VAR(AccessToken, accessToken);
DECLARE_QSTRING_VAR(RefreshToken, refreshToken);
DECLARE_INT_VAR(AnimationFrameDelay, animationFrameDelay);
DECLARE_INT_VAR(AutoSaveDelay, autoSaveDelay);
DECLARE_INT_VAR(CheckMessagesDelay, checkMessagesDelay);
DECLARE_BOOL_VAR(DisplayTimestamps, displayTimestamps);
DECLARE_BOOL_VAR(EnableAnimations, enableAnimations);
DECLARE_QSTRING_VAR(LogsDirectory, logsDirectory);
DECLARE_TYPED_VAR(DAmnTokenMethod, DAmnTokenMethod, method);
DECLARE_TYPED_VAR(QSize, WindowSize, size);
DECLARE_TYPED_VAR(QPoint, WindowPosition, position);
DECLARE_INT_VAR(Splitter, splitter);

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

	ConfigRooms m_rooms;

	bool m_modified;

	QList<QNetworkCookie> m_cookies;
};

#endif
