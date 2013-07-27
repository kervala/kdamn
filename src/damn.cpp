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
#include "damnchannel.h"
#include "damnuser.h"
#include "oauth2.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

DAmn *DAmn::s_instance = NULL;

DAmn::DAmn(QObject *parent):QObject(parent), m_socket(NULL), m_readbuffer(NULL), m_buffersize(8192)
{
	if (s_instance == NULL) s_instance = this;

	m_readbuffer = new char[m_buffersize];
}

DAmn::~DAmn()
{
	foreach(WaitingMessage *msg, m_waitingMessages) delete msg;
	foreach(DAmnChannel *channel, m_channels) delete channel;
	foreach(DAmnUser *user, m_users) delete user;

	delete [] m_readbuffer;

	s_instance = NULL;
}

bool DAmn::connectToServer()
{
	if (!m_login.isEmpty() && !m_token.isEmpty())
	{
		m_socket = new QTcpSocket(this);

		connect(m_socket, SIGNAL(connected()), this, SLOT(client()));
		connect(m_socket, SIGNAL(readyRead()), this, SLOT(read()));
		connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));

		m_socket->connectToHost("chat.deviantart.com", 3900);

		return true;
	}

	return false;
}

void DAmn::setLogin(const QString &login)
{
	m_login = login;
}

void DAmn::setToken(const QString &authtoken)
{
	m_token = authtoken;
}

void DAmn::onError(QAbstractSocket::SocketError error)
{
	qDebug() << "Error:" << error;

	emit errorReceived(tr("Socket error: %1").arg(error));
}

bool DAmn::downloadImage(const QString &url, QString &file, QString &md5)
{
	if (!OAuth2::getInstance()) return false;

	QString dir(QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)));

	md5 = QCryptographicHash::hash(url.toLatin1(), QCryptographicHash::Md5).toHex();
	QString filename = QString("%1/%2").arg(dir).arg(md5);

	static bool s_connected = false;

	if (!s_connected)
	{
		connect(OAuth2::getInstance(), SIGNAL(imageDownloaded(QString)), this, SLOT(updateWaitingMessages(QString)));
		s_connected = true;
	}

	file = QUrl::fromLocalFile(filename).toString();

	if (QFile::exists(filename)) return false;

	// create directory to be sure, there won't be any error later
	QDir tmp;
	tmp.mkpath(dir);

	OAuth2::getInstance()->get(url);

	return true;
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
			emit textReceived(msg->channel, msg->from, msg->type, msg->html, true);

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
		if (lines.size() > 0) parseAllMessages(lines);
	}

	return true;
}

DAmnChannel* DAmn::createChannel(const QString &channel)
{
	DAmnChannel *chan = getChannel(channel);

	// already exists
	if (chan) return chan;

	chan = new DAmnChannel(channel, this);

	m_channels << chan;

	return chan;
}

bool DAmn::removeChannel(const QString &channel)
{
	DAmnChannel *chan = getChannel(channel);

	// doesn't exist
	if (!chan) return false;

	m_channels.removeAll(chan);

	delete chan;

	return true;
}

DAmnChannel* DAmn::getChannel(const QString &channel)
{
	foreach(DAmnChannel *chan, m_channels) if (chan->getName().toLower() == channel.toLower()) return chan;

	return NULL;
}

DAmnUser* DAmn::createUser(const QString &user)
{
	DAmnUser *u = getUser(user);

	// already exists
	if (u) return u;

	u = new DAmnUser(user, this);

	m_users << u;

	return u;
}

DAmnUser* DAmn::getUser(const QString &user)
{
	foreach(DAmnUser *u, m_users) if (u->hasSameName(user)) return u;

	return NULL;
}
