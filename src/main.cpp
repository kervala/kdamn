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
#include "mainwindow.h"

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#ifdef QT_STATICPLUGIN

#include <QtPlugin>

#ifdef Q_OS_WIN32
	Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif
	
#ifdef Q_OS_MAC
	Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin)
#endif

	Q_IMPORT_PLUGIN(QSvgPlugin)
	Q_IMPORT_PLUGIN(QSvgIconPlugin)
	Q_IMPORT_PLUGIN(QMngPlugin)
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

#if defined(_MSC_VER) && defined(_DEBUG)
#pragma comment(linker,"/SUBSYSTEM:CONSOLE")
#endif

int main(int argv, char *args[])
{
#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	Q_INIT_RESOURCE(kdamn);

	QApplication app(argv, args);

	QApplication::setApplicationName(PRODUCT);
	QApplication::setOrganizationName(AUTHOR);
	QApplication::setApplicationVersion(VERSION);
	QApplication::setWindowIcon(QIcon(":/icons/kdamn.svg"));

	QString locale = QLocale::system().name().left(2);

	QString folder;
	QDir dir(QCoreApplication::applicationDirPath());
	
#if defined(Q_OS_WIN32)
	folder = dir.absolutePath();
#else
	dir.cdUp();

#ifdef Q_OS_MAC
	folder = dir.absolutePath() + "/Resources";
#elif defined(SHARE_PREFIX)
	folder = SHARE_PREFIX;
#else
	folder = QString("%1/share/%2").arg(dir.absolutePath()).arg(TARGET);
#endif

#endif

	folder += "/translations";

	// take the whole locale
	QTranslator localTranslator;
	if (localTranslator.load(QString("%1_%2").arg(TARGET).arg(locale), folder))
	{
		app.installTranslator(&localTranslator);
	}

	// take the whole locale
	QTranslator qtTranslator;
	if (qtTranslator.load("qt_" + locale, folder))
	{
		app.installTranslator(&qtTranslator);
	}

	MainWindow mainWindow;
	mainWindow.setWindowTitle(app.applicationName());
	mainWindow.show();

	// only memory leaks are from plugins
	return QApplication::exec();
}
