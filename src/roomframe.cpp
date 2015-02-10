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
#include "roomframe.h"
#include "moc_roomframe.cpp"
#include "configfile.h"
#include "damn.h"
#include "systrayicon.h"
#include "utils.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

RoomFrame::RoomFrame(QWidget *parent, const QString &room):TabFrame(parent), m_room(room), m_textReceived(false), m_htmlReceived(false)
{
	setupUi(this);

	m_usersModel = new QStringListModel(this);
	usersView->setModel(m_usersModel);

	QAction *homepageAction = new QAction(tr("Homepage"), this);
	connect(homepageAction, SIGNAL(triggered()), this, SLOT(onHomepageUser()));

	QAction *promoteAction = new QAction(tr("Promote"), this);
	connect(promoteAction, SIGNAL(triggered()), this, SLOT(onPromoteUser()));

	QAction *demoteAction = new QAction(tr("Demote"), this);
	connect(demoteAction, SIGNAL(triggered()), this, SLOT(onDemoteUser()));

	QAction *banAction = new QAction(tr("Ban"), this);
	connect(banAction, SIGNAL(triggered()), this, SLOT(onBanUser()));

	QAction *unbanAction = new QAction(tr("Unban"), this);
	connect(unbanAction, SIGNAL(triggered()), this, SLOT(onUnbanUser()));

	QAction *kickAction = new QAction(tr("Kick"), this);
	connect(kickAction, SIGNAL(triggered()), this, SLOT(onKickUser()));

	usersView->addAction(homepageAction);
	usersView->addAction(promoteAction);
	usersView->addAction(demoteAction);
	usersView->addAction(banAction);
	usersView->addAction(unbanAction);
	usersView->addAction(kickAction);

	outputBrowser->setRoom(room);

	m_htmlFile.setRoom(room);
	m_textFile.setRoom(room);

	updateCssFile();
	updateCssScreen();

	connect(inputEdit, SIGNAL(returnPressed()), SLOT(onSend()));
	connect(usersView, SIGNAL(doubleClicked(QModelIndex)), SLOT(onUserDoubleClicked(QModelIndex)));

	connect(outputBrowser, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(onKeyPressed(QKeyEvent*)));
}

RoomFrame::~RoomFrame()
{
}

void RoomFrame::setUsers(const QStringList &users)
{
	m_usersModel->setStringList(users);
	inputEdit->setUsers(users);

	updateSplitter();
}

void RoomFrame::userJoin(const QString &user)
{
	QStringList users = m_usersModel->stringList();

	if (users.indexOf(user) > -1) return;

	users << user;

	m_usersModel->setStringList(users);
	inputEdit->setUsers(users);

	updateSplitter();
}

void RoomFrame::userPart(const QString &user)
{
	QStringList users = m_usersModel->stringList();

	users.removeAll(user);

	m_usersModel->setStringList(users);
	inputEdit->setUsers(users);

	updateSplitter();
}

void RoomFrame::onSend()
{
	QStringList lines = inputEdit->getLines();

	if (DAmn::getInstance()->send(m_room, lines))
	{
		inputEdit->validate();
	}
}

void RoomFrame::onUserDoubleClicked(const QModelIndex &index)
{
	QString user = m_usersModel->data(index, Qt::DisplayRole).toString();

	if (user.isEmpty()) return;

	QDesktopServices::openUrl(QString("http://%1.deviantart.com").arg(user.toLower()));
}

void RoomFrame::onHomepageUser()
{
	QModelIndex index = usersView->currentIndex();

	QString user = m_usersModel->data(index, Qt::DisplayRole).toString();

	if (user.isEmpty()) return;

	QDesktopServices::openUrl(QString("http://%1.deviantart.com").arg(user.toLower()));
}

void RoomFrame::onPromoteUser()
{
	QModelIndex index = usersView->currentIndex();

	QString user = m_usersModel->data(index, Qt::DisplayRole).toString();

	if (user.isEmpty()) return;

	DAmn::getInstance()->promote(m_room, user.toLower());
}

void RoomFrame::onDemoteUser()
{

	QModelIndex index = usersView->currentIndex();

	QString user = m_usersModel->data(index, Qt::DisplayRole).toString();

	if (user.isEmpty()) return;

	DAmn::getInstance()->demote(m_room, user.toLower());
}

void RoomFrame::onBanUser()
{
	QModelIndex index = usersView->currentIndex();

	QString user = m_usersModel->data(index, Qt::DisplayRole).toString();

	if (user.isEmpty()) return;

	DAmn::getInstance()->ban(m_room, user.toLower());
}

void RoomFrame::onUnbanUser()
{
	QModelIndex index = usersView->currentIndex();

	QString user = m_usersModel->data(index, Qt::DisplayRole).toString();

	if (user.isEmpty()) return;

	DAmn::getInstance()->unban(m_room, user.toLower());
}

void RoomFrame::onKickUser()
{
	QModelIndex index = usersView->currentIndex();

	QString user = m_usersModel->data(index, Qt::DisplayRole).toString();

	if (user.isEmpty()) return;

	DAmn::getInstance()->kick(m_room, user.toLower());
}

void RoomFrame::onKeyPressed(QKeyEvent *e)
{
	QApplication::sendEvent(inputEdit, e);

	inputEdit->setFocus();
}

bool RoomFrame::setFocus(bool focus)
{
	if (!TabFrame::setFocus(focus)) return false;

	outputBrowser->setFocus(focus);

	if (m_focus) inputEdit->setFocus();

	return true;
}

void RoomFrame::updateSplitter()
{
	QStringList users = m_usersModel->stringList();

	int min = 65536;
	int max = 0;

	foreach(const QString &user, users)
	{
//		int width = usersView->fontMetrics().boundingRect(member.name).width();
		int width = usersView->fontMetrics().width(user)+10;

		if (width < min) min = width;
		if (width > max) max = width;
	}

	usersView->setMinimumWidth(min);
	usersView->setMaximumWidth(max);
}

void RoomFrame::setReceiveHtml(bool on)
{
	m_htmlReceived = on;
}

void RoomFrame::setReceiveText(bool on)
{
	m_textReceived = on;
}

void RoomFrame::appendHtml(const QString &html)
{
	outputBrowser->appendHtml(html);

	if (m_htmlReceived) m_htmlFile.append(html);
}

void RoomFrame::appendText(const QString &text)
{
	if (m_textReceived) m_textFile.append(decodeEntities(text));
}

void RoomFrame::changeEvent(QEvent *e)
{
	TabFrame::changeEvent(e);

	if (e->type() == QEvent::PaletteChange)
	{
		updateCssScreen();
	}
}

static QColor average(const QColor &color1, const QColor &color2, qreal coef)
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

void RoomFrame::updateCssScreen()
{
	QColor normalTextColor = palette().windowText().color();
	QColor normalWindowColor = palette().window().color();

	QColor darkerTextColor = average(normalTextColor, normalWindowColor, 0.75);
	QColor darkestTextColor = average(normalTextColor, normalWindowColor, 0.5);

	QColor hightlightColor = average(normalTextColor, QColor(Qt::blue), 0.5);
	QColor errorColor = average(normalTextColor, QColor(Qt::red), 0.5);

	QString css = \
	".timestamp { color: #999; }\n" \
	".username { font-weight: bold; }\n" \
	".room { }\n" \
	".privclass { font-style: italic; }\n" \
	".error { color: %3; }\n" \
	".normal { }\n" \
	".emote { font-style: italic; }\n" \
	".myself { color: %1; }\n" \
	".highlight { color: %4; font-weight: bold; }\n" \
	".system { color: %2; }\n" \
	".hidden { display: none; }\n";

	css = css.arg(darkerTextColor.name()).arg(darkestTextColor.name()).arg(errorColor.name()).arg(hightlightColor.name());

	outputBrowser->updateCss(css);
}

void RoomFrame::updateCssFile()
{
	QString css = \
	"body { background-color: #666; }\n" \
	".timestamp { color: #999; }\n" \
	".username { font-weight: bold; }\n" \
	".room { }\n" \
	".privclass { font-style: italic; }\n" \
	".error { color: #f00; }\n" \
	".normal { color: #fff; }\n" \
	".emote { font-style: italic; }\n" \
	".myself { color: #ccc; }\n" \
	".highlight { color: #ccf; font-weight: bold; }\n" \
	".system { color: #aaa; }\n";

	m_htmlFile.setCss(css);
}
