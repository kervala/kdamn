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
#include "inputedit.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

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

	emit textValidated(str);
}

void InputEdit::setUsers(const QStringList &users)
{
	m_users = users;
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
		completeName();
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
		menu->addSeparator();

		QMenu *historyMenu = menu->addMenu(tr("History"));

		foreach(const QString &history, m_history)
		{
			QAction *action = historyMenu->addAction(history);
		}

		connect(historyMenu, SIGNAL(triggered(QAction*)), this, SLOT(historyTriggered(QAction*)));
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
	QRegExp reg("[ :_,;!?.\\-]");

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
		post.clear();
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

void InputEdit::historyTriggered(QAction *action)
{
	QString text = action->text();
	setText(text);
	m_index = -1;
	m_current = text;
}

