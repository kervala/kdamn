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
#include "mainwindow.h"
#include "configfile.h"

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#ifdef QT_STATICPLUGIN

#include <QtPlugin>

#ifdef Q_OS_WIN32
	Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
	Q_IMPORT_PLUGIN(QWindowsAudioPlugin)
#endif
	
#ifdef Q_OS_MAC
	Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin)
	Q_IMPORT_PLUGIN(CoreAudioPlugin)
#endif

	Q_IMPORT_PLUGIN(QMngPlugin)
	Q_IMPORT_PLUGIN(QSvgPlugin)
	Q_IMPORT_PLUGIN(QSvgIconPlugin)
	Q_IMPORT_PLUGIN(QWebpPlugin)
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

int main(int argc, char *argv[])
{
#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	QApplication app(argc, argv);

	QApplication::setApplicationName(PRODUCT);
	QApplication::setOrganizationName(AUTHOR);
	QApplication::setApplicationVersion(VERSION);
	QApplication::setWindowIcon(QIcon(":/icons/icon.svg"));

	new ConfigFile(qApp);

	QString locale = QLocale::system().name();

	QTranslator localTranslator;
	if (localTranslator.load(QString("%1_%2").arg(TARGET).arg(locale), ConfigFile::getInstance()->getTranslationsDirectory()))
	{
		QApplication::installTranslator(&localTranslator);
	}

	QString qtbaseFilename;
#ifdef USE_QT5
	qtbaseFilename = "qtbase";
#else
	qtbaseFilename = "qt";
#endif

	QTranslator qtTranslator;
	if (qtTranslator.load(QString("%1_%2").arg(qtbaseFilename).arg(locale), ConfigFile::getInstance()->getQtTranslationsDirectory()))
	{
		QApplication::installTranslator(&qtTranslator);
	}

	MainWindow mainWindow;
	mainWindow.setWindowTitle(QApplication::applicationName());
	mainWindow.show();

	// only memory leaks are from plugins
	return QApplication::exec();
}
