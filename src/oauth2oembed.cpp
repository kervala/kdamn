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
#include "oauth2.h"
#include "damn.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

// see http://www.oembed.com

#define OEMBED_URL "http://backend.deviantart.com/oembed"

static const int s_maxWidth = 150;

bool OAuth2::isOembedUrl(const QString &url, QString *site) const
{
	if (site) *site = "deviantart";

	// DeviantART Stash
	if (QRegExp("^(http://sta.sh/([0-9a-z]+))$").exactMatch(url)) return true;

	// TODO: check against all supported sites
	// TODO: if found, return true and set siteIndex to index of the site

	return false;
}

bool OAuth2::requestOembed(const QString &url, const QString &site)
{
	if (site.isEmpty()) 
	{
		// TODO: determine site
	}

	// http://i.ytimg.com/vi/ug9bD3n3ifg/hqdefault.jpg
	// http://www.youtube.com/oembed?url=https%3A//www.youtube.com/watch%3Fv%3Dug9bD3n3ifg&format=xml&maxwidth=150&maxheight=150

	return get(QString("%1?url=%2&maxwidth=150&maxheight=150&format=json").arg(OEMBED_URL).arg(QString::fromUtf8(QUrl::toPercentEncoding(url))));
}

bool OAuth2::processOembed(const QByteArray &content, const QString &url)
{
	QString dataUrl;

	QRegExp reg("url=([a-zA-Z0-9%.]+)");

	if (reg.indexIn(url) > -1)
	{
		dataUrl = reg.cap(1);
		dataUrl = QUrl::fromPercentEncoding(dataUrl.toUtf8());
	}

	QVariantMap map;

	// we received JSON response
#ifdef USE_QT5
	QJsonParseError jsonError;
	QJsonDocument doc = QJsonDocument::fromJson(content, &jsonError);

	if (jsonError.error != QJsonParseError::NoError)
	{
		emit errorReceived(jsonError.errorString());
		return false;
	}

	map = doc.toVariant().toMap();
#else
	QScriptEngine engine;
	map = engine.evaluate("(" + QString(content) + ")").toVariant().toMap();
#endif

	// TODO: read oEmbed configuration

	QString title = map["title"].toString();
	QString author = map["author_name"].toString();
	QString providerName = map["provider_name"].toString();
	QString providerUrl = map["provider_url"].toString();
	QString version = map["version"].toString(); // "1.0"
	QString type = map["type"].toString(); // "photo" or "video"

	if (version != "1.0")
	{
		emit errorReceived(tr("Invalid oEmbed version: %1").arg(version));
		return false;
	}

	QString html;
	QString thumbnailUrl;

	int thumbWidth = 0;
	int thumbHeight = 0;

	if (type == "photo" || type == "video")
	{
		thumbnailUrl = map["thumbnail_url"].toString();
		int width = map["thumbnail_width"].toInt();
		int height = map["thumbnail_height"].toInt();

		// compute width and height of thumbnail
		if (width > s_maxWidth || height > s_maxWidth)
		{
			if (width > height)
			{
				thumbWidth = 150;
				thumbHeight = thumbWidth * height / width;
			}
			else
			{
				thumbHeight = 150;
				thumbWidth = thumbHeight * width / height;
			}
		}
		else
		{
			thumbWidth = width;
			thumbHeight = height;
		}
	}
	else if (type == "rich")
	{
		html = map["html"].toString();
		html = QString("<a href=\"%1\">%2</a><br/>%3...").arg(dataUrl).arg(title).arg(html);
	}
	else
	{
		emit errorReceived(tr("Unsupported oEmbed type: %1").arg(type));
		return false;
	}

	QMap<QString, bool> md5s;

	WaitingMessage *message = NULL;

	if (DAmn::getInstance()->getWaitingMessageFromRemoteUrl(dataUrl, message))
	{
		DAmnImagesIterator it = message->images.begin();

		while(it != message->images.end())
		{
			if (it->remoteUrl == dataUrl && it->oembed)
			{
				// replace placeholder by real values
				it->oembed = false;

				if (!thumbnailUrl.isEmpty())
				{
					// photo or video
					it->remoteUrl = thumbnailUrl;

					bool res = DAmn::getInstance()->downloadImage(*it, 100);

					// format HTML code
					html = QString("<a href=\"%3\"><img alt=\"%1\" title=\"%1\" src=\"%2\" local=\"%6\" width=\"%4\" height=\"%5\"/></a>");

					// replace Stash URL by HTML code
					message->html.replace(dataUrl, html.arg(title).arg(thumbnailUrl).arg(dataUrl).arg(thumbWidth).arg(thumbHeight).arg(it->localUrl));

					if (!res)
					{
						// already exists or network problem
						md5s[it->md5] = it->downloaded;
					}
				}
				else
				{
					// other
					message->html.replace(dataUrl, html);
					md5s[it->md5] = true;
				}
			}

			++it;
		}
	}

	QMap<QString, bool>::ConstIterator it = md5s.constBegin(), iend = md5s.constEnd();

	while(it != iend)
	{
		// update waiting list
		emit imageDownloaded(it.key(), it.value());

		++it;
	}

	return true;
}
