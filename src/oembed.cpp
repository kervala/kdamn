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
#include "oembed.h"
#include "oauth2.h"
#include "damn.h"

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

static const int s_maxWidth = 150;

// see http://www.oembed.com

OEmbed* OEmbed::s_instance = NULL;

OEmbed::OEmbed(QObject *parent)
{
	if (s_instance == NULL) s_instance = this;

	init(false);
	init(true);
}

OEmbed::~OEmbed()
{
}

void OEmbed::init(bool local)
{
	QString globalDir, localDir;

	QDir dir(QCoreApplication::applicationDirPath());
	
#if defined(Q_OS_WIN32)
	// same directory as executable
	globalDir = dir.absolutePath();
#else
	dir.cdUp();

#ifdef Q_OS_MAC
	globalDir = dir.absolutePath() + "/Resources";
#elif defined(SHARE_PREFIX)
	globalDir = SHARE_PREFIX;
#else
	globalDir = QString("%1/share/%2").arg(dir.absolutePath()).arg(TARGET);
#endif

#endif

#ifdef USE_QT5
	QStandardPaths::StandardLocation location;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
	location = QStandardPaths::AppDataLocation;
#else
	location = QStandardPaths::ConfigLocation;
#endif
	localDir = QStandardPaths::writableLocation(location);
#else
	localDir = QDesktopServices::storageLocation(QDesktopServices::AppDataLocation);
#endif

	QString filename = QString("%1/oembed.ini").arg(local ? localDir:globalDir);

	QSettings settings(filename, QSettings::IniFormat);

	QStringList groups = settings.childGroups();

	foreach(const QString &group, groups)
	{
		Site site;
		site.id = group;

		settings.beginGroup(group);
		site.name = settings.value("name").toString();
		site.schemesRegExp = settings.value("schemes").toString();
		site.schemes = site.schemesRegExp.split(' ');
		site.endpoint = settings.value("endpoint").toString();
		site.maxsize = settings.value("maxsize").toInt();
		settings.endGroup();

		// transform schemes to regular expression
		site.schemesRegExp.replace(" ", "|").replace(".", "\\.").replace("*", "(.+)"); // ([0-9a-z]+)
		site.schemesRegExp = "^(" + site.schemesRegExp + ")$";

		if (site.maxsize < s_maxWidth) site.maxsize = s_maxWidth;

		m_sites[group] = site;
	}
}

bool OEmbed::isUrlSupported(const QString &url, QString *siteId) const
{
	// try to valid each site
	foreach(const Site &s, m_sites)
	{
		if (QRegExp(s.schemesRegExp).exactMatch(url))
		{
			// return site ID
			if (siteId) *siteId = s.id;

			return true;
		}
	}

	return false;
}

bool OEmbed::request(const QString &url, const QString &siteId)
{
	Site site;

	if (siteId.isEmpty())
	{
		QString id;

		if (!isUrlSupported(url, &id)) return false;

		site = m_sites[id];
	}
	else
	{
		site = m_sites[siteId];
	}

	QString requestUrl = QString("%1?url=%3&maxwidth=%2&maxheight=%2&format=json").arg(site.endpoint).arg(site.maxsize).arg(QString::fromUtf8(QUrl::toPercentEncoding(url)));

	return OAuth2::getInstance()->getFilename(requestUrl, url);
}

bool OEmbed::commentUrl(QString &content, const QString &url) const
{
	int before = content.length();
	content.replace(url, QString("<!-- %1 -->").arg(url));
	int after = content.length();

	return after > before;
}

bool OEmbed::uncommentUrl(QString &content, const QString &url) const
{
	int before = content.length();
	content.replace(QString("<!-- %1 -->").arg(url), url);
	int after = content.length();

	return after < before;
}

bool OEmbed::replaceCommentedUrlByHtml(QString &content, const QString &url, const QString &html)
{
	int pos1 = content.indexOf(QString("<!-- %1").arg(url));
	if (pos1 < 0) return false;

	int pos2 = content.indexOf(" -->", pos1);
	if (pos2 < 0) return false;

	content.replace(pos1, pos2 + 4 - pos1, html);
	return true;
}

bool OEmbed::replaceCommentedUrlByLink(QString &content, const QString &url)
{
	int pos1 = content.indexOf(QString("<!-- %1").arg(url));
	if (pos1 < 0) return false;

	int pos2 = content.indexOf(" -->", pos1);
	if (pos2 < 0) return false;

	QString html = QString("<a href=\"%1\">%1</a>").arg(url);

	content.replace(pos1, pos2 + 4 - pos1, html);
	return true;
}

bool OEmbed::processContent(const QByteArray &content, const QString &url, const QString &filename)
{
	Data data;
	data.url = filename;

	QVariantMap map;

	// we received JSON response
#ifdef USE_QT5
	QJsonParseError jsonError;
	QJsonDocument doc = QJsonDocument::fromJson(content, &jsonError);

	if (jsonError.error != QJsonParseError::NoError)
	{
		// display wrong content too
		data.error = QString("%: %2").arg(jsonError.errorString()).arg(QString::fromUtf8(content));
	}

	map = doc.toVariant().toMap();
#else
	QScriptEngine engine;
	map = engine.evaluate("(" + QString(content) + ")").toVariant().toMap();

	if (map.isEmpty())
	{
		data.error = "Error when parsing JSON";
	}
#endif

	if (data.error.isEmpty())
	{
		data.title = map["title"].toString();
		data.author = map["author_name"].toString();
		data.providerName = map["provider_name"].toString();
		data.providerUrl = map["provider_url"].toString();

		data.version = map["version"].toString(); // "1.0"
		data.type = map["type"].toString(); // "photo", "video", "rich" or "link"

		if (data.version != "1.0")
		{
			data.error = tr("Invalid oEmbed version: %1").arg(data.version);
		}
		else
		{
			data.thumbUrl;
			data.thumbWidth = 0;
			data.thumbHeight = 0;

			if (data.type == "photo" || data.type == "video" || data.type == "rich" || data.type == "link")
			{
				data.thumbUrl = map["thumbnail_url"].toString();

				int width = map["thumbnail_width"].toInt();
				int height = map["thumbnail_height"].toInt();

				QString u = map["url"].toString();

				// bug with DA because thumbnail URL is wrong
				if (data.type == "rich" && (data.providerName == "sta.sh" || data.providerName == "DeviantArt"))
				{
					u.clear();
				}

				if (!u.isEmpty())
				{
					width = map["width"].toInt();
					height = map["height"].toInt();

					if (width && height) data.thumbUrl = u;
				}

				// compute width and height of thumbnail
				if (width > s_maxWidth || height > s_maxWidth)
				{
					if (width > height)
					{
						data.thumbWidth = s_maxWidth;
						data.thumbHeight = data.thumbWidth * height / width;
					}
					else
					{
						data.thumbHeight = s_maxWidth;
						data.thumbWidth = data.thumbHeight * width / height;
					}
				}
				else
				{
					data.thumbWidth = width;
					data.thumbHeight = height;
				}

				if (data.thumbUrl.isEmpty()) data.html = map["html"].toString();
			}
			else
			{
				data.error = tr("Unsupported oEmbed type: %1").arg(data.type);
			}
		}
	}

	return applyData(data);
}

bool OEmbed::applyData(Data &data)
{
	WaitingMessage *message = NULL;

	QMap<QString, bool> md5s;

	if (DAmn::getInstance()->getWaitingMessageFromRemoteUrl(data.url, message))
	{
		DAmnImagesIterator it = message->images.begin();

		while(it != message->images.end())
		{
			if (it->remoteUrl == data.url && it->oembed)
			{
				// replace placeholder by real values
				it->oembed = false;

				if (!data.thumbUrl.isEmpty())
				{
					// photo or video
					it->remoteUrl = data.thumbUrl;

					bool res = DAmn::getInstance()->downloadImage(*it, 100);

					if (!res)
					{
						// already exists or network problem
						md5s[it->md5] = it->downloaded;
					}
				}
				else
				{
					// other
					md5s[it->md5] = false;
				}

				// replace commented URL by HTML code
				if (buildHtml(data, it->localUrl))
				{
					replaceCommentedUrlByHtml(message->html, data.url, data.html);
				}
			}

			++it;
		}
	}
	else
	{
		qDebug() << "Error: URL not found in waiting messages";
	}

	QMap<QString, bool>::ConstIterator it = md5s.constBegin(), iend = md5s.constEnd();

	while(it != iend)
	{
		// update waiting list
		emit OAuth2::getInstance()->imageDownloaded(it.key(), it.value());

		++it;
	}

	if (!data.error.isEmpty())
	{
		emit OAuth2::getInstance()->errorReceived(data.error);
	}

	return true;
}

bool OEmbed::buildHtml(Data &data, const QString &localUrl)
{
	if (data.type == "photo" || data.type == "video" || data.type == "rich" || data.type == "link")
	{
		data.url.replace("%", "¤");

		if (!data.thumbUrl.isEmpty())
		{
			data.title.replace("\"", "&quote;");

			// format HTML code
			data.html = QString("<a href=\"%3\"><img alt=\"%1\" title=\"%1\" src=\"%2\" local=\"%6\" width=\"%4\" height=\"%5\"/></a>").arg(data.title).arg(data.thumbUrl).arg(data.url).arg(data.thumbWidth).arg(data.thumbHeight).arg(localUrl);
		}
		else if (!data.html.isEmpty())
		{
			// format text with title and preview
			data.html = QString("<a href=\"%1\">%2</a><br/>%3...").arg(data.url).arg(data.title).arg(data.html);
		}
		else if (!data.title.isEmpty())
		{
			// format text with title and preview
			data.html = QString("<a href=\"%1\">%2</a>").arg(data.url).arg(data.title);
		}
		else
		{
			// simple URL
			data.html = QString("<a href=\"%1\">%1</a>").arg(data.url);
		}

		data.url.replace("¤", "%");
		data.html.replace("¤", "%");

		return true;
	}

	return false;
}

