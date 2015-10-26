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
#include "utils.h"

static QMap<QChar, QString> s_specialEntities;
static QString s_userAgent;
static QString s_imagesFilter;

void initSpecialEntities()
{
	if (!s_specialEntities.isEmpty()) return;

	s_specialEntities['<'] = "lt";
	s_specialEntities['>'] = "gt";
	s_specialEntities['&'] = "amp";
}

/***************************************************************************//*!
 * @brief Encode all non ASCII characters into &#...;
 * @param[in] src        Text to analyze
 * @param[in,opt] force  Force the characters "list" to be converted.
 * @return ASCII text compatible.
 *
 * @note Original code: http://www.qtforum.org/article/3891/text-encoding.html
 *
 * @warning Do not forget to use QString::fromUtf8()
 */
QString encodeEntities(const QString& src, const QString& force)
{
	initSpecialEntities();

	QString tmp(src);
	uint len = tmp.length();
	uint i = 0;

	while(i < len)
	{
		if (tmp[i].unicode() > 128 || force.contains(tmp[i]))
		{
			QString ent;

			if (s_specialEntities.contains(tmp[i]))
			{
				// look first for named entities
				ent = s_specialEntities[tmp[i]];
			}
			else
			{
				// use unicode value
				ent = "#" + QString::number(tmp[i].unicode());
			}

			QString rp = "&" + ent + ";";
			tmp.replace(i, 1, rp);
			len += rp.length()-1;
			i += rp.length();
		}
		else
		{
			++i;
		}
	}

	return tmp;
}

/***************************************************************************//*!
 * @brief Allows decode &#...; into UNICODE (utf8) character.
 * @param[in] src    Text to analyze
 * @return UNICODE (utf8) text.
 *
 * @note Do not forget to include QRegExp
 */
QString decodeEntities(const QString& src)
{
	initSpecialEntities();

	QString ret(src);
	QRegExp re("&#([0-9]+);");
	re.setMinimal(true);

	int pos = 0;

	while((pos = re.indexIn(src, pos)) != -1)
	{
		ret = ret.replace(re.cap(0), QChar(re.cap(1).toInt(0, 10)));
		pos += re.matchedLength();
	}

	return ret;
}

QString convertDateToISO(const QString &date)
{
	// Oct 30, 2014, 1:50:33 PM
	QString format = "MMM d, yyyy, h:mm:ss AP";
	QDateTime dateTime = QLocale(QLocale::English).toDateTime(date, format);
	QString iso = dateTime.toString(Qt::ISODate);
	return iso.replace("T", " ");
}

QString convertIDOToDate(const QString &date)
{
	QString iso = date;
	iso.replace("T", " ");
	QDateTime valid = QDateTime::fromString(iso, Qt::ISODate);

	return valid.toString(Qt::DefaultLocaleShortDate);
}

QString base36enc(qint64 value)
{
	static const QString base36("0123456789abcdefghijklmnopqrstuvwxyz");

	QString res;

	do
	{
		res.prepend(base36[(int)(value % 36)]);
	}
	while (value /= 36);

	return res;
}

QColor average(const QColor &color1, const QColor &color2, qreal coef)
{
	QColor c1 = color1.toHsv();
	QColor c2 = color2.toHsv();

	qreal h = -1.0;

	if (c1.hsvHueF() == -1.0)
	{
		h = c2.hsvHueF();
	}
	else if (c2.hsvHueF() == -1.0)
	{
		h = c1.hsvHueF();
	}
	else
	{
		h = ((1.0 - coef) * c2.hsvHueF()) + (coef * c1.hsvHueF());
	}

	qreal s = ((1.0 - coef) * c2.hsvSaturationF()) + (coef * c1.hsvSaturationF());
	qreal v = ((1.0 - coef) * c2.valueF()) + (coef * c1.valueF());

	return QColor::fromHsvF(h, s, v).toRgb();
}

QString GetUserAgent()
{
	if (s_userAgent.isEmpty())
	{
		QString system;
#ifdef Q_OS_WIN32
		system = "Windows ";

		switch (QSysInfo::WindowsVersion)
		{
			case QSysInfo::WV_32s: system += "3.1 with Win32s"; break;
			case QSysInfo::WV_95: system += "95"; break;
			case QSysInfo::WV_98: system += "98"; break;
			case QSysInfo::WV_Me: system += "Me"; break;
			case QSysInfo::WV_DOS_based: system += "DOS"; break;

			case QSysInfo::WV_4_0: system += "NT 4.0"; break; // Windows NT 4
			case QSysInfo::WV_5_0: system += "NT 5.0"; break; // Windows 2000
			case QSysInfo::WV_5_1: system += "NT 5.1"; break; // Windows XP
			case QSysInfo::WV_5_2: system += "NT 5.2"; break; // Windows XP 64bits
			case QSysInfo::WV_6_0: system += "NT 6.0"; break; // Windows Vista
			case QSysInfo::WV_6_1: system += "NT 6.1"; break; // Windows 7
			case QSysInfo::WV_6_2: system += "NT 6.2"; break; // Windows 8
			case QSysInfo::WV_6_3: system += "NT 6.3"; break; // Windows 8.1
//			case QSysInfo::WV_10_0: system += "NT 10.0"; break; // Windows 10
			case QSysInfo::WV_NT_based: system += "NT"; break;

			case QSysInfo::WV_CE: system += "CE"; break;
			case QSysInfo::WV_CENET: system += "CE Net"; break;
			case QSysInfo::WV_CE_5: system += "CE 5"; break;
			case QSysInfo::WV_CE_6: system += "CE 6"; break;
			case QSysInfo::WV_CE_based: system += "CE"; break;
		}

		system += "; ";

		// Windows target processor
		system += QString("Win%1").arg(IsOS64bits() ? 64:32);

		system += "; ";

		// application target processor
#ifdef _WIN64
		system += "x64; ";
#else
		system += "i386;";
#endif

		system += QLocale::system().name().replace('_', '-');
#else
#endif
		s_userAgent = QString("%1/%2 (%3)").arg(QApplication::applicationName()).arg(QApplication::applicationVersion()).arg(system);
	}

	return s_userAgent;
}

QString GetSupportedImageFormatsFilter()
{
	if (s_imagesFilter.isEmpty())
	{
		QList<QByteArray> formats = QImageReader::supportedImageFormats();

		foreach(const QByteArray &format, formats)
		{
			if (!s_imagesFilter.isEmpty()) s_imagesFilter += "|";

			s_imagesFilter += format;
		}

		s_imagesFilter = "(" + s_imagesFilter + ")";
	}

	return s_imagesFilter;
}
