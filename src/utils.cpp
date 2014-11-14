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
#include "utils.h"

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
	QString tmp(src);
	uint len = tmp.length();
	uint i = 0;

	while(i < len)
	{
		if (tmp[i].unicode() > 128 || force.contains(tmp[i]))
		{
			QString rp = "&#" + QString::number(tmp[i].unicode()) + ";";
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
