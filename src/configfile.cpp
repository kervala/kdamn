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
#include "configfile.h"
#include "moc_configfile.cpp"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

#define IMPLEMENT_SIMPLE_VAR(type, function, var) \
void ConfigFile::set##function(type var)\
{\
	if (m_##var == var) return;\
	\
	m_##var = var;\
	modified(true);\
}\
\
type ConfigFile::get##function() const\
{\
	return m_##var;\
}

#define IMPLEMENT_TYPED_VAR(type, function, var) \
void ConfigFile::set##function(const type &var)\
{\
	if (m_##var == var) return;\
	\
	m_##var = var;\
	modified(true);\
}\
\
type ConfigFile::get##function() const\
{\
	return m_##var;\
}

#define IMPLEMENT_QSTRING_VAR(function, var) \
IMPLEMENT_TYPED_VAR(QString, function, var)

#define IMPLEMENT_QCOLOR_VAR(function, var) \
IMPLEMENT_TYPED_VAR(QColor, function, var)

#define IMPLEMENT_INT_VAR(function, var) \
IMPLEMENT_SIMPLE_VAR(int, function, var)

#define IMPLEMENT_BOOL_VAR(function, var) \
IMPLEMENT_SIMPLE_VAR(bool, function, var)

ConfigFile* ConfigFile::s_instance = NULL;

ConfigFile::ConfigFile(QObject* parent):QObject(parent), m_settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName())
{
	m_rememberPassword = true;
	m_animationFrameDelay = 100; // in ms
	m_autoSaveDelay = 30; // in seconds
	m_checkMessagesDelay = 60; // in seconds
	m_displayTimestamps = true;
	m_enableAnimations = true; // enable animations for MNG and animated GIF
	m_splitter = 0; // position of splitter
	m_enableOembedThumbnail = true;
	m_enableLogs = true;
	m_enableTextLogs = true;
	m_enableHtmlLogs = true;
	m_enableSound = false;
	m_useSystray = false;
	m_hideMinimizedWindow = false;

	if (!s_instance) s_instance = this;

	initDirectories();
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

	if (version == 2)
	{
		loadVersion2();
	}
	else
	{
		loadVersion1();
	}

	autoSave();

	return true;
}

bool ConfigFile::loadVersion1()
{
	// server parameters
	m_login = m_settings.value("login").toString();
	m_password = m_settings.value("password").toString();
	m_rememberPassword = m_settings.value("remember_password", true).toBool();

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
	// general parameters
	m_autoSaveDelay = m_settings.value("autosave").toInt();

	// server parameters
	m_settings.beginGroup("server");

	m_login = m_settings.value("login").toString();
	m_password = m_settings.value("password").toString();
	m_rememberPassword = m_settings.value("remember_password", true).toBool();

	m_settings.endGroup();

	// window parameters
	m_settings.beginGroup("window");

	m_size = QSize(m_settings.value("width", 0).toInt(), m_settings.value("height", 0).toInt());
	m_position = QPoint(m_settings.value("x", 0).toInt(), m_settings.value("y", 0).toInt());
	m_splitter = m_settings.value("splitter", 0).toInt();

	m_settings.endGroup();

	// chat parameters
	m_settings.beginGroup("chat");

	m_enableAnimations = m_settings.value("enable_animations", true).toBool();
	m_animationFrameDelay = m_settings.value("animation_frame_delay", 100).toInt();
	m_displayTimestamps = m_settings.value("display_timestamps", true).toBool();
	m_useSystray = m_settings.value("use_systray", false).toBool();
	m_hideMinimizedWindow = m_settings.value("hide_minimized_windows", false).toBool();
	m_enableOembedThumbnail = m_settings.value("enable_oembed_thumbnail", true).toBool();
	m_highlightColor = m_settings.value("highlight_color", "").toString();
	m_errorColor = m_settings.value("error_color", "").toString();
	m_screenStyle = m_settings.value("screen_style", "").toString();
	m_logStyle = m_settings.value("log_style", "").toString();

	m_settings.endGroup();

	// logs parameters
	m_settings.beginGroup("logs");

	m_enableLogs = m_settings.value("enable_logs", true).toBool();
	m_enableHtmlLogs = m_settings.value("enable_html_logs", true).toBool();
	m_enableTextLogs = m_settings.value("enable_text_logs", true).toBool();
	m_logsDirectory = m_settings.value("logs_directory", "").toString();

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

	// messages parameters
	m_settings.beginGroup("messages");

	m_checkMessagesDelay = m_settings.value("check_messages_delay", 60).toInt(); // in seconds

	m_settings.endGroup();

	// sounds parameters
	m_settings.beginGroup("sounds");

	m_enableSound = m_settings.value("enable_sounds", true).toBool();
	m_nameMentionedSound = m_settings.value("name_mentioned_sound", "").toString();
	m_noteReceivedSound = m_settings.value("note_received_sound", "").toString();

	m_settings.endGroup();

	updateSettings();

	return true;
}

bool ConfigFile::save()
{
	// no need to save because no change has been made
	if (!m_modified)
	{
		autoSave();
		return true;
	}

	// clear previous entries
	m_settings.clear();

	// general parameters
	m_settings.setValue("version", 2);
	m_settings.setValue("autosave", m_autoSaveDelay);

	// server parameters
	m_settings.beginGroup("server");

	m_settings.setValue("login", m_login);
	m_settings.setValue("password", m_password);
	m_settings.setValue("remember_password", m_rememberPassword);

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

	m_settings.setValue("enable_animations", m_enableAnimations);
	m_settings.setValue("animation_frame_delay", m_animationFrameDelay);
	m_settings.setValue("display_timestamps", m_displayTimestamps);
	m_settings.setValue("use_systray", m_useSystray);
	m_settings.setValue("hide_minimized_windows", m_hideMinimizedWindow);
	m_settings.setValue("enable_oembed_thumbnail", m_enableOembedThumbnail);
	m_settings.setValue("highlight_color", m_highlightColor == Qt::blue ? "":m_highlightColor.name());
	m_settings.setValue("error_color", m_errorColor == Qt::red ? "":m_errorColor.name());
	m_settings.setValue("screen_style", m_screenStyle);
	m_settings.setValue("log_style", m_logStyle);

	m_settings.endGroup();

	// logs parameters
	m_settings.beginGroup("logs");

	m_settings.setValue("enable_logs", m_enableLogs);
	m_settings.setValue("enable_html_logs", m_enableHtmlLogs);
	m_settings.setValue("enable_text_logs", m_enableTextLogs);
	m_settings.setValue("logs_directory", m_logsDirectory == m_defaultLogsDirectory ? "":m_logsDirectory);

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

	// messages parameters
	m_settings.beginGroup("messages");

	m_settings.setValue("check_messages_delay", m_checkMessagesDelay);

	m_settings.endGroup();

	// sounds parameters
	m_settings.beginGroup("sounds");

	m_settings.setValue("enable_sounds", m_enableSound);
	m_settings.setValue("name_mentioned_sound", m_nameMentionedSound);
	m_settings.setValue("note_received_sound", m_noteReceivedSound);

	m_settings.endGroup();

	modified(false);

	autoSave();

	return true;
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

void ConfigFile::initDirectories()
{
	// logs directory
	QString documentsPath;

#ifdef USE_QT5
	documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#else
	documentsPath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#endif

	// define default logs directory
	m_defaultLogsDirectory = QString("%1/kdAmn/logs").arg(documentsPath);

	if (!QDir().mkpath(m_defaultLogsDirectory))
	{
		qCritical() << "Unable to create directory" << m_defaultLogsDirectory;
	}

	QDir applicationDir(QApplication::applicationDirPath());

	// global data directory
#if defined(Q_OS_WIN32)
#ifdef _DEBUG
	applicationDir.cdUp();
	applicationDir.cdUp();

	// translations are in build directory
	QString buildDirectory = applicationDir.absolutePath();

	// all files are in misc subfolder
	applicationDir.cdUp();
	applicationDir.cd("misc");
#endif
	// same directory as executable
	m_globalDataDirectory = applicationDir.absolutePath();
#else

#ifdef Q_OS_MAC
	applicationDir.cdUp();
	m_globalDataDirectory = applicationDir.absolutePath() + "/Resources";
#else
	// Linux
	QStringList directoriesToSearch;

#ifdef SHARE_PREFIX
	directoriesToSearch << SHARE_PREFIX;
#endif

	// application directory
	directoriesToSearch << applicationDir.absolutePath();

	// application parent directory
	applicationDir.cdUp();
	directoriesToSearch << applicationDir.absolutePath();

	// application share/kdamn directory
	directoriesToSearch << QString("%1/share/%2").arg(applicationDir.absolutePath()).arg(TARGET);;

	// current path
	directoriesToSearch << QDir::currentPath();

	foreach(const QString &directory, directoriesToSearch)
	{
		if (QFile::exists(QString("%1/oembed.ini").arg(directory)))
		{
			m_globalDataDirectory = directory;
			break;
		}
	}

	if (m_globalDataDirectory.isEmpty())
	{
		qDebug() << "Unable to find a directory with resources!";
	}
#endif

#endif

	// translations directory
#if defined(Q_OS_WIN32) && defined(_DEBUG)
	m_translationsDirectory = buildDirectory + "/translations";
#else
	m_translationsDirectory = m_globalDataDirectory + "/translations";
#endif

	// Qt translations directory
#if defined(Q_OS_WIN32)
	m_qtTranslationsDirectory = m_translationsDirectory;
#elif defined(Q_OS_MAC)
	m_qtTranslationsDirectory = m_translationsDirectory;
#elif defined(USE_QT5)
	m_qtTranslationsDirectory = "/usr/share/qt5/translations";
#else
	m_qtTranslationsDirectory = "/usr/share/qt4/translations";
#endif

	// cache directory
#ifdef USE_QT5
	m_cacheDirectory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
	m_cacheDirectory = QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
#endif

	// local data
#ifdef USE_QT5
	QStandardPaths::StandardLocation location;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
	location = QStandardPaths::AppDataLocation;
#else
	location = QStandardPaths::ConfigLocation;
#endif
	m_localDataDirectory = QStandardPaths::writableLocation(location);
#else
	m_localDataDirectory = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif

	// download directory
#ifdef USE_QT5
	m_downloadDirectory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
#else
	m_downloadDirectory = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#endif


}

void ConfigFile::updateSettings()
{
	// logs directory
	if (m_logsDirectory.isEmpty() || !QFile::exists(m_logsDirectory))
	{
		m_logsDirectory = m_defaultLogsDirectory;
	}

	// colors
	if (!m_highlightColor.isValid())
	{
		m_highlightColor = Qt::blue;
	}

	if (!m_errorColor.isValid())
	{
		m_errorColor = Qt::red;
	}
}

void ConfigFile::autoSave()
{
	if (m_autoSaveDelay > 0) QTimer::singleShot(m_autoSaveDelay * 60 * 1000, this, SLOT(save()));
}

void ConfigFile::modified(bool modified)
{
	m_modified = modified;
}

IMPLEMENT_QSTRING_VAR(Login, login);
IMPLEMENT_QSTRING_VAR(Password, password);
IMPLEMENT_BOOL_VAR(RememberPassword, rememberPassword);
IMPLEMENT_INT_VAR(AnimationFrameDelay, animationFrameDelay);
IMPLEMENT_INT_VAR(AutoSaveDelay, autoSaveDelay);
IMPLEMENT_INT_VAR(CheckMessagesDelay, checkMessagesDelay);
IMPLEMENT_BOOL_VAR(DisplayTimestamps, displayTimestamps);
IMPLEMENT_BOOL_VAR(EnableAnimations, enableAnimations);
IMPLEMENT_QSTRING_VAR(DefaultLogsDirectory, defaultLogsDirectory);
IMPLEMENT_QSTRING_VAR(LogsDirectory, logsDirectory);
IMPLEMENT_QSTRING_VAR(TranslationsDirectory, translationsDirectory);
IMPLEMENT_QSTRING_VAR(QtTranslationsDirectory, qtTranslationsDirectory);
IMPLEMENT_QSTRING_VAR(CacheDirectory, cacheDirectory);
IMPLEMENT_QSTRING_VAR(DownloadDirectory, downloadDirectory);
IMPLEMENT_QSTRING_VAR(GlobalDataDirectory, globalDataDirectory);
IMPLEMENT_QSTRING_VAR(LocalDataDirectory, localDataDirectory);
IMPLEMENT_INT_VAR(Splitter, splitter);
IMPLEMENT_BOOL_VAR(EnableOembedThumbnail, enableOembedThumbnail);
IMPLEMENT_BOOL_VAR(EnableLogs, enableLogs);
IMPLEMENT_BOOL_VAR(EnableTextLogs, enableTextLogs);
IMPLEMENT_BOOL_VAR(EnableHtmlLogs, enableHtmlLogs);
IMPLEMENT_QCOLOR_VAR(HighlightColor, highlightColor);
IMPLEMENT_QCOLOR_VAR(ErrorColor, errorColor);
IMPLEMENT_QSTRING_VAR(ScreenStyle, screenStyle);
IMPLEMENT_QSTRING_VAR(LogStyle, logStyle);
IMPLEMENT_BOOL_VAR(EnableSound, enableSound);
IMPLEMENT_QSTRING_VAR(NameMentionedSound, nameMentionedSound);
IMPLEMENT_QSTRING_VAR(NoteReceivedSound, noteReceivedSound);
IMPLEMENT_BOOL_VAR(UseSystray, useSystray);
IMPLEMENT_BOOL_VAR(HideMinimizedWindow, hideMinimizedWindow);
