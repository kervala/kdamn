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

	QRegExp reg = filterRegExp();

	return note.date.contains(reg)
		|| note.sender.contains(reg)
		|| note.subject.contains(reg)
		|| note.preview.contains(reg);
}
