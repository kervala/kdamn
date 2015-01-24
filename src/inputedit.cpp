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
#include "inputedit.h"
#include "damn.h"
#include "damnuser.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

#define MAX_HISTORY 40

InputEdit::InputEdit(QWidget *parent):QLineEdit(parent), m_index(-1)
{
}

void InputEdit::validate()
{
	QString str = text().trimmed();

	if (str.isEmpty()) return;

	// don't add to history if already the last entry
	if (m_history.isEmpty() || m_history.back() != str) m_history << str;

	clear();

	m_index = -1;

	emit textValidated(str);
}

void InputEdit::setUsers(const QStringList &users)
{
	m_users = users;

	// remove current user to avoid autocompleting our own name
	QString login = DAmn::getInstance()->getLogin();
	if (login.isEmpty()) return;

	DAmnUser *user = DAmn::getInstance()->getUser(login);
	if (!user) return;

	QString name = user->getName();
	if (name.isEmpty()) return;

	m_users.removeAll(name);
}

QStringList InputEdit::getLines() const
{
	QStringList lines = text().split('\n');

	if (lines.size() == 1 && lines[0].isEmpty()) lines.clear();

	for(int i = 0; i < lines.size(); ++i)
	{
		if (lines[i].isEmpty()) lines[i] = "\t";
		else if (lines[i].right(1) == " ") lines[i].truncate(lines[i].length()-1);
	}

	return lines;
}

void InputEdit::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Up)
	{
		historyUp();
	}
	else if (e->key() == Qt::Key_Down)
	{
		historyDown();
	}
	else if (e->key() == Qt::Key_Tab)
	{
		QString str = text();

		if (!str.isEmpty())
		{
			QRegExp reg("^/([a-z]+)");

			int pos = reg.indexIn(str);

			if (pos == 0 && cursorPosition() <= reg.matchedLength())
			{
				completeCommand();
			}
			else
			{
				completeName();
			}
		}
	}
	else if (e->key() == Qt::Key_PageUp)
	{
		emit pageUp();
	}
	else if (e->key() == Qt::Key_PageDown)
	{
		emit pageDown();
	}
	else
	{
		if (m_index > -1 && text() != m_history[m_index]) m_index = -1;

		// default handler for event
		QLineEdit::keyPressEvent(e);
	}
}

void InputEdit::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = createStandardContextMenu();

	if (!menu) return;

	menu->setAttribute(Qt::WA_DeleteOnClose);

	if (!m_history.isEmpty())
	{
		QAction *action = NULL;

		menu->addSeparator();

		QMenu *historyMenu = menu->addMenu(tr("History"));
	
		for(int i = 0; i < m_history.size(); ++i)
		{
			QString history = m_history[i];

			if (history.length() > MAX_HISTORY)
			{
				int pos = history.lastIndexOf(QRegExp("[ -_\n\t:;]"), MAX_HISTORY);

				if (pos == -1) pos = MAX_HISTORY-3;

				history = history.left(pos) + "...";
			}

			action = historyMenu->addAction(history);

			action->setData(i);
		}

		connect(historyMenu, SIGNAL(triggered(QAction*)), this, SLOT(historyTriggered(QAction*)));

		action = menu->addAction(tr("Clear history"));

		connect(action, SIGNAL(triggered()), this, SLOT(clearHistory()));
	}

	menu->popup(e->globalPos());
}

bool InputEdit::event(QEvent *e)
{
	// redirect all keypresses without exception to keyPressEvent
	if (e->type() == QEvent::KeyPress)
	{
		keyPressEvent((QKeyEvent *)e);

		return true;
	}

	return QLineEdit::event(e);
}

void InputEdit::historyUp()
{
	// move back in history
	if (!m_history.isEmpty() && (m_index > 0 || m_index == -1))
	{
		if (m_index == -1)
		{
			m_current = text();
			m_index = m_history.size()-1;
		}
		else
		{
			--m_index;
		}

		setText(m_history[m_index]);
	}
}

void InputEdit::historyDown()
{
	// move forward in history
	if (!m_history.isEmpty() && m_index != -1)
	{
		if (m_index == m_history.size()-1)
		{
			m_index = -1;

			setText(m_current);
		}
		else
		{
			++m_index;

			setText(m_history[m_index]);
		}
	}
}

void InputEdit::completeName()
{
	const QRegExp reg("[^a-zA-Z0-9-]");

	// whole string
	QString str = text();

	// beginning of username
	int pos1 = 0;

	// end of username
	int pos2 = str.indexOf(reg, cursorPosition());

	// text before and after username
	QString pre, post;

	// separator after username
	QString sep = " ";
	
	if (pos2 == -1)
	{
		// no separator after username, take end of line
		pos2 = str.length();
	}
	else
	{
		sep = str[pos2];
		post = str.mid(pos2+1);
	}

	pos1 = str.lastIndexOf(reg, pos2-1)+1;

	pre = str.left(pos1);

	int len = pos2-pos1;

	if (len > 0)
	{
		QString prefix = str.mid(pos1, len).toLower();

		foreach(const QString &user, m_users)
		{
			if (user.toLower().startsWith(prefix))
			{
				if (pos1 == 0 && post.isEmpty()) sep = ":" + sep;
				
				setText(pre + user + sep + post);

				setCursorPosition(pos1 + user.length() + sep.length());

				break;
			}
		}
	}
}

void InputEdit::completeCommand()
{
	static const QString sCommands[] =
	{
		"me",
		"part",
		"whois",
		"topic",
		"title",
		"join",
		"part",
		"demote",
		"promote",
		"kick",
		"ban",
		"unban",
		"admin",
		"clear",
		"stats",
		"raw",
		""
	};

	// whole string without "/"
	QString str = text().mid(1);

	for(int i = 0; !sCommands[i].isEmpty(); ++i)
	{
		if (str == sCommands[i].left(str.length()))
		{
			QString cmd = QString("/%1 ").arg(sCommands[i]);

			setText(cmd);
			setCursorPosition(cmd.length());

			break;
		}
	}
}

void InputEdit::historyTriggered(QAction *action)
{
	QString text = m_history[action->data().toInt()];
	setText(text);
	m_index = -1;
	m_current = text;
}

void InputEdit::clearHistory()
{
	m_history.clear();
	m_index = -1;
}
