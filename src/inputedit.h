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

#ifndef INPUTEDIT_H
#define INPUTEDIT_H

#include <QLineEdit>

class InputEdit : public QLineEdit
{
	Q_OBJECT

public:
	InputEdit(QWidget *parent);
	
	void validate();
	void setUsers(const QStringList &users);

	QStringList getLines() const;

signals:
	void textValidated(const QString &text);
	void pageUp();
	void pageDown();
	
public slots:
	void historyUp();
	void historyDown();
	void completeName();
	void completeCommand();
	void historyTriggered(QAction *action);
	void clearHistory();

protected:
	typedef QMap<QString, QString> UsersMap;

	void keyPressEvent(QKeyEvent *event);
	void contextMenuEvent(QContextMenuEvent *e);
	bool event(QEvent *e);
	bool findWordBefore(const QString &text, int position, QString &word, int &beginningPosition, int &endPosition) const;
	bool cycleUser(const QString &user, QString &res) const;

	QStringList m_history;
	int m_index;
	QString m_current;
	UsersMap m_users;
};

#endif
