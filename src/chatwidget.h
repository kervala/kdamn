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

#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QTextBrowser>

class ChatWidget : public QTextBrowser
{
	Q_OBJECT

public:
	ChatWidget(QWidget *parent);
	virtual ~ChatWidget();

	void setAction(const QString &user, const QString &text);
	void setText(const QString &user, const QString &text);
	void setSystem(const QString &text);
	void setError(const QString &text);

	QVariant loadResource(int type, const QUrl &name);

	void startAnimations(const QString &html);
	bool addAnimation(const QString &url, const QString &file);

	void setFocus(bool focus);

	QString getRoom() const { return m_room; }
	void setRoom(const QString &room) { m_room = room; }

public slots:
	// when user click on a link
	void onUrl(const QUrl &url);

	void animate(int frame);

protected:
	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dragLeaveEvent(QDragLeaveEvent *event);
	void dropEvent(QDropEvent *event);

	QString getTimestamp() const;

	QHash<QMovie*, QUrl> m_urls;
	bool m_focus;
	QString m_room;
};

class AnimationStart : public QObject
{
	Q_OBJECT

public:
	AnimationStart(const QString &url, const QString &file, ChatWidget *widget);

public slots:
	void timeout();

private:
	ChatWidget *m_widget;
	QString m_url;
	QString m_file;
};

#endif
