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
#include "damnroom.h"
#include "damnuser.h"
#include "oauth2.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

DAmn *DAmn::s_instance = NULL;

DAmn::DAmn(QObject *parent):QObject(parent), m_socket(NULL)
{
	if (s_instance == NULL) s_instance = this;
}

DAmn::~DAmn()
{
	foreach(WaitingMessage *msg, m_waitingMessages) delete msg;
	foreach(DAmnRoom *room, m_rooms) delete room;
	foreach(DAmnUser *user, m_users) delete user;

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

bool DAmn::downloadImage(DAmnImage &image)
{
	if (!OAuth2::getInstance()) return false;

	static bool s_connected = false;

	if (!s_connected)
	{
		connect(OAuth2::getInstance(), SIGNAL(imageDownloaded(QString)), this, SLOT(updateWaitingMessages(QString)));
		s_connected = true;
	}

	image.downloaded = false;
	image.valid = false;

	QByteArray ext;
	int pos = image.remoteUrl.lastIndexOf('.');

	if (pos == -1) return false;

	QRegExp reg(OAuth2::getSupportedImageFormatsFilter());

	image.valid = reg.indexIn(image.remoteUrl.mid(pos+1).toLatin1()) == 0;

	if (!image.valid) return false;

	QString cachePath;

#ifdef USE_QT5
	cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
	cachePath = QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
#endif

	QString dir(QDir::fromNativeSeparators(cachePath));

	image.md5 = QCryptographicHash::hash(image.remoteUrl.toLatin1(), QCryptographicHash::Md5).toHex();
	image.filename = QString("%1/%2").arg(dir).arg(image.md5);
	image.localUrl = QUrl::fromLocalFile(image.filename).toString();
	image.downloaded = QFile::exists(image.filename);

	if (image.downloaded) return false;

	// create directory to be sure, there won't be any error later
	QDir tmp;
	tmp.mkpath(dir);

	// request download of image
	OAuth2::getInstance()->get(image.remoteUrl);

	return true;
}

bool DAmn::updateWaitingMessages(const QString &md5)
{
	QString filename;
	QStringList rooms;

	foreach(WaitingMessage *msg, m_waitingMessages)
	{
		if (!msg) continue;

		int total = 0;
		int downloaded = 0;

		// mark downloaded all images with the same md5
		DAmnImagesIterator it = msg->images.begin();

		while(it != msg->images.end())
		{
			++total;

			if (md5 == it->md5)
			{
				++downloaded;

				if (!it->downloaded)
				{
					it->downloaded = true;
					filename = it->localUrl;
				}

				if (!rooms.contains(msg->room)) rooms << msg->room;
			}

			++it;
		}

		// check if all images have been downloaded
		if (total == downloaded)
		{
			emit textReceived(msg->room, msg->from, msg->type, msg->html, true);

			m_waitingMessages.removeAll(msg);

			delete msg;
		}
	}

	return true;
}

bool DAmn::read()
{
	QByteArray data = m_socket->readAll();

	// split all packets
	QList<QByteArray> packets = data.split(0);

	foreach(const QByteArray &packet, packets)
	{
		if (packet.isEmpty()) continue;

		QStringList lines = QString::fromLatin1(packet).split("\n");

		// at least one line
		if (lines.size() > 0) parseAllMessages(lines);
	}

	return true;
}

DAmnRoom* DAmn::createRoom(const QString &room)
{
	DAmnRoom *chan = getRoom(room);

	// already exists
	if (chan) return chan;

	chan = new DAmnRoom(room, this);

	m_rooms << chan;

	return chan;
}

bool DAmn::removeRoom(const QString &room)
{
	DAmnRoom *chan = getRoom(room);

	// doesn't exist
	if (!chan) return false;

	m_rooms.removeAll(chan);

	delete chan;

	return true;
}

DAmnRoom* DAmn::getRoom(const QString &room)
{
	foreach(DAmnRoom *chan, m_rooms) if (chan->getName().toLower() == room.toLower()) return chan;

	return NULL;
}

DAmnUser* DAmn::createUser(const QString &user)
{
	DAmnUser *u = getUser(user);

	// already exists
	if (u) return u;

	// create new user
	u = new DAmnUser(user, this);

	// add him to the list
	m_users << u;

	return u;
}

DAmnUser* DAmn::getUser(const QString &user)
{
	foreach(DAmnUser *u, m_users) if (u->hasSameName(user)) return u;

	return NULL;
}
