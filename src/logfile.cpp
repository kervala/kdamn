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
#include "logfile.h"
#include "configfile.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

LogFile::LogFile()
{
}

LogFile::~LogFile()
{
	close();
}

void LogFile::setRoom(const QString &room)
{
	m_room = room;
}

void LogFile::setCss(const QString &css)
{
	m_css = css;
}

void LogFile::append(const QString &text)
{
	if (!m_file.isOpen()) open();

	if (m_file.isOpen()) m_file.write(text.toUtf8() + "\n");
}

void LogFile::open()
{
	QString filename = QString("%1/%2-%3").arg(ConfigFile::getInstance()->getLogsDirectory()).arg(m_room).arg(QDateTime::currentDateTime().toString(Qt::ISODate).replace(":", "-"));

	if (!m_css.isEmpty())
	{
		filename += ".htm";
	}
	else
	{
		filename += ".txt";
	}

	m_file.setFileName(filename);

	if (m_file.open(QIODevice::Append))
	{
	}
	else
	{
		qCritical() << "Unable to open file" << filename;
	}
}

void LogFile::close()
{
	if (!m_file.isOpen()) return;

	if (!m_css.isEmpty())
	{
		m_file.write("</body></html>");
		m_file.close();

		if (m_file.open(QFile::ReadOnly))
		{
			QByteArray content = m_file.readAll();

			m_file.close();

			if (m_file.open(QFile::WriteOnly))
			{
				// prepend HTML header with inline CSS
				QString header;
				header += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
				header += "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n";
				header += "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n";
				header += "<head>\n";
				header += "<style type=\"text/css\">\n";
				header += m_css;
				header += "</style>\n";
				header += "</head>\n";
				header += "<body>\n";

				m_file.write(header.toUtf8());
				m_file.write(content);
			}
		}
	}

	m_file.close();
}
