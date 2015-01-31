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

#ifndef OEMBED_H
#define OEMBED_H

class OEmbed : public QObject
{
	Q_OBJECT

	struct Site
	{
		QString id;
		QString name;
		QStringList schemes;
		QString schemesRegExp;
		QString endpoint;
		int maxsize;

		Site():maxsize(150)
		{
		}
	};

	typedef QMap<QString, Site> Sites;

	struct Data
	{
		QString url;
		QString html;
		QString title;
		QString author;
		QString providerName;
		QString providerUrl;

		QString version;
		QString type;

		QString thumbUrl;
		int thumbWidth;
		int thumbHeight;

		QString error;

		Data():thumbWidth(0), thumbHeight(0)
		{
		}
	};

public:
	OEmbed(QObject *parent);
	virtual ~OEmbed();

	static OEmbed* getInstance() { return s_instance; }

	bool isUrlSupported(const QString &url, QString *site = NULL) const;
	bool request(const QString &url, const QString &site = "");

	bool commentUrl(QString &content, const QString &url) const; // URL => <!-- URL -->
	bool uncommentUrl(QString &content, const QString &url) const; // <!-- URL --> => URL

	bool replaceCommentedUrlByHtml(QString &content, const QString &url, const QString &html);
	bool replaceCommentedUrlByLink(QString &content, const QString &url);

	bool processContent(const QByteArray &content, const QString &url, const QString &filename);
	bool applyData(Data &data);
	bool buildHtml(Data &data, const QString &localUrl);

private:
	void init(bool local);

	Sites m_sites;

	static OEmbed *s_instance;
};

#endif
