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
	m_users.clear();

	foreach(const QString &user, users)
	{
		// key is lower case version of user name
		m_users[user.toLower()] = user;
	}

	// remove current user to avoid autocompleting our own name
	QString login = DAmn::getInstance()->getLogin();
	if (login.isEmpty()) return;

	DAmnUser *user = DAmn::getInstance()->getUser(login);
	if (!user) return;

	QString name = user->getName();
	if (name.isEmpty()) return;

	m_users.remove(name.toLower());
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
	bool found = false;

	if (e->modifiers() == Qt::NoModifier)
	{
		if (e->key() == Qt::Key_Up)
		{
			historyUp();

			found = true;
		}
		else if (e->key() == Qt::Key_Down)
		{
			historyDown();

			found = true;
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

				found = true;
			}
		}
		else if (e->key() == Qt::Key_PageUp)
		{
			emit pageUp();

			found = true;
		}
		else if (e->key() == Qt::Key_PageDown)
		{
			emit pageDown();

			found = true;
		}
	}

	if (!found)
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
	// whole string
	QString str = text();

	// beginning and end of username
	int beginning, end;

	QString word;
	
	if (!findWordBefore(str, cursorPosition(), word, beginning, end)) return;

	// look in users list
	QString user;

	// found at least one user that matchs word
	if (!cycleUser(word, user)) return;

	// text before and after username
	QString pre = str.left(beginning);
	QString post = str.mid(end+1);

	QString nextText;
	QString sep;

	if (beginning == 0 && (post.isEmpty() || post == ": " || post == ":"))
	{
		sep = ": ";
		post.clear();
	}
	else if (post.isEmpty() || post == " ")
	{
		sep = " ";
		post.clear();
	}

	setText(pre + user + sep + post);
	setCursorPosition(beginning + user.length() + sep.length());
}

bool InputEdit::findWordBefore(const QString &text, int position, QString &word, int &beginningPosition, int &endPosition) const
{
	// regular expression to detect a separator
	const QRegExp specialReg("[^a-zA-Z0-9\\-]");
	const QRegExp alphaReg("[a-zA-Z0-9\\-]");

	// if position is at the end or character at position is a special character
	if (position >= text.length() || specialReg.exactMatch(text[position]))
	{
		// look for last alpha character before position
		endPosition = text.lastIndexOf(alphaReg, position);

		// no alpha characters in text
		if (endPosition == -1) return false;
	}
	else
	{
		// look for separator after position
		endPosition = text.indexOf(specialReg, position + 1);

		// end not found, take string to the end
		if (endPosition == -1)
		{
			endPosition = text.length();
		}

		--endPosition;
	}

	beginningPosition = text.lastIndexOf(specialReg, endPosition);

	// beginning not found
	if (beginningPosition == -1)
	{
		// take string from the beginning
		beginningPosition = 0;
	}
	else
	{
		// take first alpha character
		++beginningPosition;
	}

	// compute size of word
	int size = endPosition - beginningPosition + 1;

	// return word
	word = text.mid(beginningPosition, size);

	return true;
}

bool InputEdit::cycleUser(const QString &user, QString &res) const
{
	QString lowerUser = user.toLower();

	UsersMap::const_iterator it = m_users.begin();
	UsersMap::const_iterator iend = m_users.end();

	// compare string with user names
	while(it != iend)
	{
		// at least partial match
		if (it.key().startsWith(lowerUser))
		{
			if (it.key() != lowerUser)
			{
				// partial match, return full user name
				res = it.value();
			}
			else
			{
				QString currentUser = it.value();

				// exact match
				++it;

				if (it == m_users.end())
				{
					// end of users list, return the first one
					res = m_users.first();

					// if user name is exactly the same
					if (res == currentUser) return false;
				}
				else
				{
					// return the next one
					res = it.value();
				}
			}

			return true;
		}

		++it;
	}

	return false;
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
		"waiting",
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
