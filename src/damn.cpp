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
#include "damn.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

static const QString s_exprUser = "([A-Za-z0-9_-]+)";
static const QString s_exprChannel = "([A-Za-z0-9_.-]+)";
static const QString s_exprCommand = "([a-zA-Z]+)";
static const QString s_exprParameter = "([A-Za-z0-9_.~ -]*)";
static const QString s_exprProperty = "([a-z]+)";
static const QString s_exprTablump = "((/?)([a-z/]+))";

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
//	{ "iframe", 3, "iframe" },
	{ "iframe", 3, "a" },
	{ "link", 2, "a" },
	{ "a", 1, "a" },
	{ "", -1 }
};

/*
/abbr:		</abbr>
/acro:		</acronym>
/embed:		</embed>
*/
DAmn::DAmn(QObject *parent):QObject(parent), m_socket(NULL), m_manager(NULL), m_step(eStepNone), m_readbuffer(NULL), m_buffersize(8192)
{
	m_readbuffer = new char[m_buffersize];

	m_manager = new QNetworkAccessManager(this);

	connect(m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onReply(QNetworkReply*)));
	connect(m_manager, SIGNAL(proxyAuthenticationRequired(QNetworkProxy, QAuthenticator *)), SLOT(onAuthentication(QNetworkProxy, QAuthenticator *)));
}

DAmn::~DAmn()
{
	foreach(WaitingMessage *msg, m_waitingMessages)
	{
		delete msg;
	}

	foreach(DAmnChannel *channel, m_channels)
	{
		delete channel;
	}

	foreach(DAmnUser *user, m_users)
	{
		delete user;
	}

	delete [] m_readbuffer;
}

bool DAmn::connectToDA()
{
	if (!m_login.isEmpty() && !m_authtoken.isEmpty())
	{
		m_step = eStepAuthToken;

		return connectToChat();
	}

	if (!m_login.isEmpty() && !m_password.isEmpty())
	{
		return requestCookies();
	}

	emit authenticationRequired();

	return false;
}

bool DAmn::requestCookies()
{
	QString siteurl("https://www.deviantart.com/users/login");

//	QList<QNetworkCookie> cookies;
//	cookies << QNetworkCookie("skipintro", "1");

	QNetworkRequest req;
	req.setUrl(QUrl(siteurl));
//	req.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.2.8) Gecko/20100731 Firefox/3.6.8 (Swiftfox)");
//	req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(cookies));
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	req.setRawHeader("Referer", "http://www.deviantart.com/users/rockedout");
//	req.setRawHeader("Connection", "close");
//	req.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
//	req.setRawHeader("Accept-Language", "fr,fr-fr;q=0.8,en-us;q=0.5,en;q=0.3");
//	req.setRawHeader("Accept-Charset", "ISO-8859-1,utf-8;q=0.7,*;q=0.7");

	QUrlQuery params;
	params.addQueryItem("ref", siteurl);
	params.addQueryItem("username", m_login);
	params.addQueryItem("password", m_password);
	params.addQueryItem("remember_me", "1");

	QByteArray data = params.query().toUtf8();

	QNetworkReply *reply = m_manager->post(req, data);

	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onReplyError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));

	return true;
}

bool DAmn::requestAuthToken()
{
	QString siteurl("http://chat.deviantart.com/chat/Botdom");

	QNetworkRequest req;
	req.setUrl(QUrl(siteurl));
//	req.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.2.8) Gecko/20100731 Firefox/3.6.8 (Swiftfox)");
//	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	req.setRawHeader("Referer", "http://chat.deviantart.com");

	QNetworkReply *reply = m_manager->get(req);

	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onReplyError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));

	return true;
}

bool DAmn::connectToChat()
{
	m_socket = new QTcpSocket(this);

	connect(m_socket, SIGNAL(connected()), this, SLOT(client()));
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(read()));
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));

	m_socket->connectToHost("chat.deviantart.com", 3900);

	return true;
}

void DAmn::setLogin(const QString &login)
{
	m_login = login;
}

void DAmn::setPassword(const QString &password)
{
	m_password = password;
}

void DAmn::setAuthtoken(const QString &authtoken)
{
	m_authtoken = authtoken;
}

void DAmn::onError(QAbstractSocket::SocketError error)
{
	qDebug() << "Error:" << error;
}

void DAmn::onReplyError(QNetworkReply::NetworkError error)
{
	qDebug() << "Error:" << error;
}

void DAmn::onReply(QNetworkReply *reply)
{
	if (reply->error() == QNetworkReply::NoError)
	{
	}

	// get returned cookies
//	QString cookies = QUrl::fromPercentEncoding(reply->rawHeader("Set-Cookie"));

	if (/*!cookies.isEmpty() && */ m_step == eStepNone)
	{
		m_step = eStepCookies;

		requestAuthToken();
	}
	else if (m_step == eStepCookies)
	{
		QRegExp reg("dAmn_Login\\( \"" + s_exprUser + "\", \"([a-f0-9]{32})\"");

		QString html = reply->readAll();

		if (reg.indexIn(html) > -1)
		{
			m_login = reg.cap(1);
			m_authtoken = reg.cap(2);

			m_step = eStepAuthToken;

			emit authtokenReceived(m_authtoken);

			connectToChat();
		}
	}
	else if (m_step == eStepConnected)
	{
//		QString redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl().toString();
		QByteArray content = reply->readAll();
		QString url = reply->url().toString();
		QString md5 = QCryptographicHash::hash(url.toLatin1(), QCryptographicHash::Md5).toHex();

		QDir dir;
		dir.mkpath("cache");

		QFile file("cache/" + md5);

		if (file.open(QIODevice::WriteOnly))
		{
			file.write(content);
		}

		updateWaitingMessages(md5);
	}
	
	reply->deleteLater();
}

bool DAmn::updateWaitingMessages(const QString &md5)
{
	foreach(WaitingMessage *msg, m_waitingMessages)
	{
		if (!msg) continue;

		// remove all images with the same md5
		msg->images.removeAll(md5);

		// check if all images have been downloaded
		if (msg->images.isEmpty())
		{
			if (msg->action)
			{
				emit htmlActionReceived(msg->channel, msg->from, msg->html);
				emit textActionReceived(msg->channel, msg->from, msg->html);
			}
			else
			{
				emit htmlMessageReceived(msg->channel, msg->from, msg->html);
				emit textMessageReceived(msg->channel, msg->from, msg->html);
			}

			m_waitingMessages.removeAll(msg);

			delete msg;
		}
	}

	return true;
}

void DAmn::onSslErrors(const QList<QSslError> &errors)
{
	qDebug() << "SSL Errors:" << errors;
}

void DAmn::onAuthentication(const QNetworkProxy &proxy, QAuthenticator *auth)
{
	qDebug() << "auth";
}

bool DAmn::read()
{
	qint64 size = 0;
	bool end = false;

	do
	{
		size += m_socket->read(m_readbuffer + size, m_buffersize - size);

		if (size >= m_buffersize)
		{
			qDebug() << "buffer too large";
		}

		char c = m_readbuffer[size-1];

		end = (c == '\0');
	}
	while(!end);

//	QString data = QString::fromUtf8(m_socket->readAll());
	QString data = QString::fromLatin1(m_readbuffer, size);

	qDebug() << data;

	QStringList lines = data.split("\n");

	// at least one line
	if (lines.size() < 1) return false;

	return parseAllMessages(lines);
}

bool DAmn::replaceTablumps(const QString &data, QString &html, QString &text, QStringList &images)
{
	int pos1 = 0, pos2 = 0;

	QRegExp reg("&" + s_exprTablump + "\t");

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
						qDebug() << "bad regexp";
					}
				}

				break;
			}
		}

		if (!found)
		{
			qDebug() << id << "not found";
		}
		else
		{
			if (!close && tokens.size() == tab.count)
			{
				if (id == "emote")
				{
					QString alt = tokens[1];
					QString width = tokens[2];
					QString height = tokens[3];
					QString title = tokens[4];
					QString url = "http://e.deviantart.net/emoticons/" + tokens[5];

					QString md5 = QCryptographicHash::hash(url.toLatin1(), QCryptographicHash::Md5).toHex();
					QString file = "cache/" + md5;

					if (!QFile::exists(file))
					{
						downloadImage(url);

						images << md5;
					}

					html += QString("<img alt=\"%1\" width=\"%2\" height=\"%3\" title=\"%4\" src=\"%5\" />").arg(alt).arg(width).arg(height).arg(title).arg(file);
					text += alt;
				}
				else if (id == "dev")
				{
					QChar symbol = tokens[1][0];
					QString name = tokens[2];

					html += QString("%1<a href=\"http://%3.deviantart.com\">%2</a>").arg(symbol).arg(name).arg(name.toLower());
					text += symbol+name;
				}
				else if (id == "avatar")
				{
					QString name = tokens[1].toLower();
					int format = tokens[2].toInt();

					QString ext;
				
					if (format == 11 || format == 15 || format == 35 || format == 7 || format == 27 || format == 23 || format == 31 || format == 59)
					{
						ext = "png";
					}
					else if (format == 13 || format == 33 || format == 5 || format == 41 || format == 25 || format == 45 || format == 49 || format == 37 || format == 9 || format == 17 || format == 21)
					{
						ext = "gif";
					}
					else if (format == 42 || format == 22 || format == 6 || format == 62 || format == 30 || format == 26 || format == 14 || format == 34)
					{
						ext = "jpg";
					}
					else
					{
						qDebug() << name << format;
					}

					QString first = name[0];
					QString second = name[1];

					QRegExp reg2("^([a-z0-9_])$");

					if (reg2.indexIn(first))
					{
						first = "_";
					}

					if (reg2.indexIn(second))
					{
						second = "_";
					}

					QString url = QString("http://a.deviantart.net/avatars/%1/%2/%3.%4").arg(first).arg(second).arg(name).arg(ext);

					QString md5 = QCryptographicHash::hash(url.toLatin1(), QCryptographicHash::Md5).toHex();
					QString file = "cache/" + md5;

					if (!QFile::exists(file))
					{
						downloadImage(url);

						images << md5;
					}

					html += QString("<a href=\"http://%1.deviantart.com\"><img alt=\"%1\" width=\"50\" height=\"50\" title=\"%1\" src=\"%2\" /></a>").arg(name).arg(file);
					text += QString(":icon%1:").arg(name);
				}
				else if (id == "thumb")
				{
					QString id = tokens[1];
					QString title = tokens[2];
					QString resolution = tokens[3];
					QString nimp1 = tokens[4]; // 4
					QString url = tokens[5];
					QString nimp2 = tokens[6]; // 0:2:0

					QStringList tokens = url.split(':');

					url = QString("http://th02.deviantart.net/%1/150/%2").arg(tokens[0]).arg(tokens[1]); // PRE

					QString md5 = QCryptographicHash::hash(url.toLatin1(), QCryptographicHash::Md5).toHex();
					QString file = "cache/" + md5;

					if (!QFile::exists(file))
					{
						downloadImage(url);

						images << md5;
					}

					QString link = QString("http://www.deviantart.com/art/%1-%2").arg(title.replace(" ", "-")).arg(id);

					html += QString("<a href=\"%3\"><img alt=\"%1\" src=\"%2\" /></a>").arg(title).arg(file).arg(link);
					text += QString(":thumb%1:").arg(id);
				}
				else if (id == "iframe")
				{
					QString url = tokens[1];
					QString width = tokens[2];
					QString height = tokens[3];

//					html += QString("<iframe src=\"%1\" width=\"%2\" height=\"%3\">").arg(url).arg(width).arg(height);
					html += QString("<a href=\"%1\">%1").arg(url);
					text += url;
				}
				else if (id == "img")
				{
					QString url = tokens[1];
					QString width = tokens[2];
					QString height = tokens[3];

					html += QString("<img src=\"%1\" alt=\"\" width=\"%2\" height=\"%3\" />").arg(url).arg(width).arg(height);
					text += url;
				}
				else if (id == "link")
				{
					QString url = tokens[1];
					QString title = tokens[2];

					if (title == "&")
					{
						html += QString("<a href=\"%1\">%1</a>").arg(url);
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
					qDebug() << id << "not recognized";
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

	return images.isEmpty();
}

bool DAmn::downloadImage(const QString &url)
{
	QNetworkRequest req;
	req.setUrl(QUrl(url));

	QNetworkReply *reply = m_manager->get(req);

	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onReplyError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));

	return true;
}

DAmnChannel* DAmn::createChannel(const QString &channel)
{
	DAmnChannel *chan = getChannel(channel);

	// already exists
	if (chan) return chan;

	chan = new DAmnChannel();
	chan->name = channel;

	m_channels << chan;

	return chan;
}

DAmnChannel* DAmn::getChannel(const QString &channel)
{
	foreach(DAmnChannel *chan, m_channels) if (chan->name.toLower() == channel.toLower()) return chan;

	return NULL;
}

DAmnUser* DAmn::createUser(const QString &user)
{
	DAmnUser *u = getUser(user);

	// already exists
	if (u) return u;

	u = new DAmnUser();
	u->name = user;

	m_users << u;

	return u;
}

DAmnUser* DAmn::getUser(const QString &user)
{
	foreach(DAmnUser *u, m_users) if (u->name.toLower() == user.toLower()) return u;

	return NULL;
}
