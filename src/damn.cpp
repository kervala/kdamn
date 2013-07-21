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

DAmn *DAmn::s_instance = NULL;

static const QString s_exprUser = "([A-Za-z0-9_-]+)";
static const QString s_exprChannel = "([A-Za-z0-9_.-]+)";
static const QString s_exprCommand = "([a-zA-Z]+)";
static const QString s_exprParameter = "([A-Za-z0-9_.~ -]*)";
static const QString s_exprProperty = "([a-z]+)";
static const QString s_exprTablump = "((/?)([a-z/]+))";

DAmn::DAmn(QObject *parent):QObject(parent), m_socket(NULL), m_manager(NULL), m_step(eStepNone), m_readbuffer(NULL), m_buffersize(8192)
{
	if (s_instance == NULL) s_instance = this;

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

	s_instance = NULL;
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

	emit errorReceived(tr("Socket error: %1").arg(error));
}

void DAmn::onReplyError(QNetworkReply::NetworkError error)
{
	qDebug() << "Error:" << error;

	emit errorReceived(tr("Network error: %1").arg(error));
}

void DAmn::onSslErrors(const QList<QSslError> &errors)
{
	qDebug() << "SSL Errors:" << errors;

	foreach(const QSslError &error, errors)
	{
		emit errorReceived(tr("SSL errors: %1").arg(error.errorString()));
	}
}

void DAmn::onAuthentication(const QNetworkProxy &proxy, QAuthenticator *auth)
{
	qDebug() << "auth";

	emit errorReceived(tr("Authentication required"));
}

void DAmn::onReply(QNetworkReply *reply)
{
	if (reply->error() == QNetworkReply::NoError)
	{
	}

	if (m_step == eStepNone)
	{
		// use returned cookies for other requests
		m_step = eStepCookies;

		requestAuthToken();
	}
	else if (m_step == eStepCookies)
	{
		QRegExp reg("dAmn_Login\\( \"([A-Za-z0-9_-]+)\", \"([a-f0-9]{32})\"");

		QString html = reply->readAll();

		if (reg.indexIn(html) > -1)
		{
			m_login = reg.cap(1);
			m_authtoken = reg.cap(2);

			m_step = eStepAuthToken;

			emit authtokenReceived(m_login, m_authtoken);

			connectToChat();
		}
	}
	else if (m_step == eStepConnected)
	{
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

bool DAmn::read()
{
	qint64 size = 0;

	do
	{
		size += m_socket->read(m_readbuffer + size, m_buffersize - size);

		// no data
		if (size == 0) return false;

		// to much data
		if (size >= m_buffersize)
		{
			emit errorReceived(tr("Buffer too large (> 8192)"));
		}
	}
	while(m_readbuffer[size-1]);

//	QString data = QString::fromUtf8(m_socket->readAll());
	QString data = QString::fromLatin1(m_readbuffer, size-1);

	// split all packets
	QStringList packets = data.split(QChar(0));

	foreach(const QString &packet, packets)
	{
		QStringList lines = packet.split("\n");

		// at least one line
		if (lines.size() < 1) return false;

		if (!parseAllMessages(lines)) return false;
	}

	return true;
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
