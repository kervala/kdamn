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
	QString str = text();

	// don't add to history if already the last entry
	if (!str.isEmpty() && (m_history.isEmpty() || m_history.back() != str)) m_history << str;

	clear();
}

void InputEdit::keyPressEvent(QKeyEvent *event)
{
	if(event->key() == Qt::Key_Up)
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
	else if(event->key() == Qt::Key_Down)
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
	else
	{
		if (m_index > -1 && text() != m_history[m_index]) m_index = -1;

		// default handler for event
		QLineEdit::keyPressEvent(event);
	}
}
