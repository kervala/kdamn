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
#include "damn.h"
#include "oauth2.h"
#include "oembed.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

static Tablump s_tablumps[] = {
	{ "emote", 5 , "" },
	{ "thumb", 6 , "" },
	{ "avatar", 2, "" },
	{ "dev", 2, "" },
	{ "b", 0, "b" },
	{ "i", 0, "em" },
	{ "u", 0, "u" },
	{ "sub", 0, "sub" },
	{ "sup", 0, "sup" },
	{ "s", 0, "del" },
	{ "p", 0, "p" },
	{ "br", 0, "br" },
	{ "code", 0, "code" },
	{ "bcode", 0, "pre code" },
	{ "li", 0, "li" },
	{ "ul", 0, "ul" },
	{ "ol", 0, "ol" },
	{ "img", 3, "img", },
//	{ "iframe", 3, "iframe" },
//	{ "embed", 3, "embed" },
	{ "iframe", 3, "a" },
	{ "embed", 3, "a" },
	{ "link", 2, "a" },
	{ "a", 1, "a" },
	{ "abbr", 1, "abbr" },
	{ "acro", 1, "acronym" },
	{ "", -1 }
};

bool DAmn::replaceTablumps(const QString &data, QString &html, QString &text, DAmnImages &images)
{
	int pos1 = 0, pos2 = 0;

	QRegExp reg("&((/?)([a-z/]+))\t");

	while((pos1 = reg.indexIn(data, pos2)) > -1)
	{
		// copy all text from last position
		html += data.mid(pos2, pos1 - pos2);
		text += data.mid(pos2, pos1 - pos2);

		pos2 = pos1 + reg.matchedLength();

		QString id = reg.cap(3);
		bool close = reg.cap(2) == "/";

		Tablump tab;

		QStringList tokens;
		bool found = false;

		for(int i = 0; s_tablumps[i].count != -1; ++i)
		{
			tab = s_tablumps[i];

			if (id == tab.id)
			{
				found = true;

				if (tab.count > 0 && !close)
				{
					// only open tags have parameters
					QString regStr = QString("([^\t]*)\t").repeated(tab.count);
					QRegExp regTab(regStr);

					if (regTab.indexIn(data, pos2) == pos2)
					{
						tokens = regTab.capturedTexts();

						pos2 += regTab.matchedLength();
					}
					else
					{
						emit errorReceived(tr("Unable to parse tablump: %1").arg(data));
					}
				}

				break;
			}
		}

		if (!found)
		{
			emit errorReceived(tr("Tablump %1 not found in table").arg(id));
		}
		else
		{
			if (!close && (tokens.size() == tab.count + 1 || (!tab.count && !tokens.size())))
			{
				if (id == "emote")
				{
					QString alt = tokens[1];
					QString width = tokens[2];
					QString height = tokens[3];
					QString title = tokens[4];
					QString url = "http://e.deviantart.net/emoticons/" + tokens[5];

					DAmnImage image;
					image.originalUrl = url;
					image.remoteUrl = url;
					image.oembed = false;

					if (downloadImage(image) && !images.contains(image)) images << image;

					html += QString("<img alt=\"%1\" width=\"%2\" height=\"%3\" title=\"%4\" src=\"%5\" local=\"%6\" />").arg(alt).arg(width).arg(height).arg(title).arg(image.remoteUrl).arg(image.localUrl);
					text += alt;
				}
				else if (id == "dev")
				{
					QChar symbol = tokens[1][0];
					QString name = tokens[2];

					html += QString("%1<a href=\"http://%3.deviantart.com\">%2</a>").arg(symbol).arg(name).arg(name.toLower());
					text += QString(":dev%1:").arg(name);
				}
				else if (id == "avatar")
				{
					QString name = tokens[1];
					int usericon = tokens[2].toInt();

					QString url = getAvatarUrl(name.toLower(), usericon);

					DAmnImage image;
					image.originalUrl = url;
					image.remoteUrl = url;
					image.oembed = false;

					if (downloadImage(image) && !images.contains(image)) images << image;

					html += QString("<a href=\"http://%1.deviantart.com\"><img alt=\"%1\" width=\"50\" height=\"50\" title=\"%1\" src=\"%2\" local=\"%3\" /></a>").arg(name).arg(image.remoteUrl).arg(image.localUrl);
					text += QString(":icon%1:").arg(name);
				}
				else if (id == "thumb")
				{
					QString number = tokens[1];
					QString title = tokens[2];
					QString resolution = tokens[3];
					QString tnserver = tokens[4];
					QString url = tokens[5];
					QString flags = tokens[6];

					bool noshadow = false; // don't paint a shadow
					bool mature = false; // only display link and 'mature deviation'
					bool nopriv = false; // only display link

					int width = 0;
					int height = 0;

					QString pre, post;

					tokens = resolution.split('x');

					if (tokens.size() == 2)
					{
						width = tokens[0].toInt();
						height = tokens[1].toInt();
					}

					tokens = flags.split(':');

					if (tokens.size() == 3)
					{
						noshadow = tokens[0].toInt() > 0;
						mature = tokens[1].toInt() > 0;
						nopriv = tokens[2].toInt() > 0;
					}

					QString title2 = title;
					title2.replace(QRegExp("[^a-zA-Z0-9]+"), "-");

					QString link = QString("http://www.deviantart.com/art/%1-%2").arg(title2).arg(number);

					// journal entry
					if (url == "images.deviantart.com/shared/poetry.jpg")
					{
						html += QString("<a href=\"%1\">%2</a>").arg(link).arg(title);
					}
					else
					{
						tokens = url.split(':');

						if (tokens.size() == 2)
						{
							pre = tokens[0];
							post = tokens[1];
						}

						bool useOEmbed = pre.startsWith("s3");

						if (!useOEmbed)
						{
							if (width > 150 || height > 150)
							{
								url = QString("http://th%1.deviantart.net/%2/150/%3");

								if (height > width)
								{
									width = width * 150 / height;
									height = 150;
								}
								else
								{
									height = height * 150 / width;
									width = 150;
								}
							}
							else
							{
								url = QString("http://fc%1.deviantart.net/%2/%3");
							}

							url = url.arg(tnserver, 2, '0').arg(pre).arg(post);

							DAmnImage image;
							image.originalUrl = url;
							image.remoteUrl = url;
							image.oembed = false;

							if (downloadImage(image) && !images.contains(image)) images << image;

							html += QString("<a href=\"%3\"><img alt=\"%1\" title=\"%1\" src=\"%2\" local=\"%6\" width=\"%4\" height=\"%5\"/></a>").arg(title).arg(image.remoteUrl).arg(link).arg(width).arg(height).arg(image.localUrl);
						}
						else
						{
							url = QString("http://www.deviantart.com/deviation/%1").arg(number);

							QString oembedSite;

							// check if we should use oembed
							if (OEmbed::getInstance()->isUrlSupported(url, &oembedSite))
							{
								DAmnImage image;
								image.originalUrl = url;
								image.remoteUrl = url;
								image.oembed = true;
								image.oembedSite = oembedSite;

								// to check if duplicated
								image.md5 = url;

								// request image info using oembed
								if (OEmbed::getInstance()->request(image.remoteUrl, oembedSite))
								{
									QString formattedUrl = url;

									// comment URL to know we're will fix it later
									OEmbed::commentUrl(formattedUrl, url);

									// URL will be replaced by HTML code later
									html += formattedUrl;

									if (!images.contains(image)) images << image;
								}
								else
								{
									// to process normally
									oembedSite.clear();
								}
							}

							if (oembedSite.isEmpty())
							{
								// normal link (not oEmbed)
								html += QString("<a href=\"%1\">%1</a>").arg(url);
							}
						}
					}

					text += QString(":thumb%1:").arg(number);
				}
				else if (id == "iframe" || id == "embed")
				{
					QString url = tokens[1];
					QString width = tokens[2];
					QString height = tokens[3];

					html += QString("<a href=\"%1\">%1").arg(url);
					text += url;
				}
				else if (id == "img")
				{
					QString url = tokens[1];
					QString width = tokens[2];
					QString height = tokens[3];

					DAmnImage image;
					image.originalUrl = url;
					image.remoteUrl = url;
					image.oembed = false;

					if (downloadImage(image) && !images.contains(image)) images << image;

					html += QString("<img src=\"%1\" local=\"%4\" alt=\"\" width=\"%2\" height=\"%3\" />").arg(image.remoteUrl).arg(width).arg(height).arg(image.localUrl);
					text += url;
				}
				else if (id == "link")
				{
					QString url = tokens[1];
					QString title = tokens[2];

					if (title == "&")
					{
						QString oembedSite;

						// check if we should use oembed
						if (OEmbed::getInstance()->isUrlSupported(url, &oembedSite))
						{
							DAmnImage image;
							image.originalUrl = url;
							image.remoteUrl = url;
							image.oembed = true;
							image.oembedSite = oembedSite;

							// to check if duplicated
							image.md5 = url;

							// request image info using oembed
							if (OEmbed::getInstance()->request(image.remoteUrl, oembedSite))
							{
								QString formattedUrl = url;

								// comment URL to know we're will fix it later
								OEmbed::commentUrl(formattedUrl, url);

								// URL will be replaced by HTML code later
								html += formattedUrl;

								if (!images.contains(image)) images << image;
							}
							else
							{
								// to process normally
								oembedSite.clear();
							}
						}

						if (oembedSite.isEmpty())
						{
							// normal link (not oEmbed)
							html += QString("<a href=\"%1\">%1</a>").arg(url);
						}
					}
					else
					{
						html += QString("<a href=\"%1\" title=\"%2\">%2</a>").arg(url).arg(title);
						pos2 += 2; // skip following "&\t"
					}

					text += url;
				}
				else if (id == "a")
				{
					QString url = tokens[1];

					html += QString("<a href=\"%1\">").arg(url);
					text += url;
				}
				else if (id == "abbr" || id == "acro")
				{
					QString title = tokens[1];

					html += QString("<%1 title=\"%2\">").arg(tab.tag).arg(title);
					text += QString("(%1) ").arg(title);
				}
				else if (!tab.tag.isEmpty() && !tab.count)
				{
					QStringList tags = tab.tag.split(' ');

					for(int i = 0; i < tags.size(); ++i)
					{
						html += QString("<%1>").arg(tags[i]);
					}
				}
				else
				{
					emit errorReceived(tr("Tablump %1 not recognized").arg(id));
				}
			}
			else
			{
				// closed tags don't need parameters
				QStringList tags = tab.tag.split(' ');

				for(int i = tags.size()-1; i >= 0; --i)
				{
					html += QString("</%1>").arg(tags[i]);
				}
			}
		}
	}

	html += data.mid(pos2);
	text += data.mid(pos2);

	return images.isEmpty();
}

QString DAmn::getAvatarUrl(const QString &user, int usericon)
{
	QString url;

	int cachebuster = (usericon >> 2) & 15;
	int format = usericon & 3;

	if (format < 1 || format > 3)
	{
		url = "http://a.deviantart.net/avatars/default.gif";
	}
	else
	{
		static const QString s_formats[] = { "", "gif", "jpg", "png" };

		QString ext = s_formats[format];

		QChar first = user[0];
		QChar second = user[1];

		QRegExp reg2("^([a-z0-9_])$");

		if (reg2.indexIn(first))
		{
			first = '_';
		}

		if (reg2.indexIn(second))
		{
			second = '_';
		}

		url = QString("http://a.deviantart.net/avatars/%1/%2/%3.%4").arg(first).arg(second).arg(user).arg(ext);
	}

	if (cachebuster) url += QString("?%1").arg(cachebuster);

	return url;
}
