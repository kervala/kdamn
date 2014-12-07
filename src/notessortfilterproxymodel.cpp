/*
 *  kdAmn is a deviantART Messaging Network client
 *  Copyright (C) 2013-2014  Cedric OCHS
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
#include "notessortfilterproxymodel.h"
#include "notesmodel.h"
#include "moc_notessortfilterproxymodel.cpp"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

NotesSortFilterProxyModel::NotesSortFilterProxyModel(QObject *parent):QSortFilterProxyModel(parent)
{
}

bool NotesSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	NotesModel *model = qobject_cast<NotesModel*>(sourceModel());

	const Note &note = model->getNote(sourceRow);

	return note.date.contains(filterRegExp())
		|| note.preview.contains(filterRegExp())
		|| note.sender.contains(filterRegExp())
		|| note.subject.contains(filterRegExp());
}

bool NotesSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	NotesModel *model = qobject_cast<NotesModel*>(sourceModel());

	int column = left.column();

	if (right.column() != column)
	{
		qDebug() << "Error different columns";
	}

	const Note &leftNote = model->getNote(left.row());
	const Note &rightNote = model->getNote(right.row());

	switch(column)
	{
		case 0:
		return leftNote.subject < rightNote.subject;

		case 1:
		return leftNote.sender < rightNote.sender;
		
		case 2:
		return leftNote.date < rightNote.date;

		default:
		break;
	}

	return false;
}
