/*
 *  FileChecker is a tool to find duplicates and bad files
 *  Copyright (C) 2010  Cedric OCHS
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
 *  $Id: importdialog.h 255 2009-08-02 20:59:17Z kervala $
 *
 */

#include "common.h"
#include "notesmodel.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

NotesModel::NotesModel(QObject *parent):QAbstractTableModel(parent), m_waitingData(false)
{
}

NotesModel::~NotesModel()
{
	m_folder.save();
}

void NotesModel::setFolder(const Folder &folder)
{
	beginResetModel();

	m_folder = folder;

	endResetModel();

	m_waitingData = false;
}

void NotesModel::updateFolder(const Folder &folder, int offset, int count)
{
	// prevent inserting wrong rows
	if (offset < m_folder.notes.size()) return;

	beginInsertRows(QModelIndex(), offset, offset + count - 1);

	m_folder = folder;

	endInsertRows();

	m_waitingData = false;
}

int NotesModel::rowCount(const QModelIndex &/* parent */) const
{
	return m_folder.notes.size();
}

int NotesModel::columnCount(const QModelIndex &/* parent */) const
{
	return 3;
}

QVariant NotesModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= m_folder.notes.size() || index.row() < 0 || role != Qt::DisplayRole) return QVariant();

	const Note &note = m_folder.notes[index.row()];

	switch(index.column())
	{
		case 0:
		return note.subject;

		case 1:
		return note.sender;

		case 2:
		return note.date;

		default:
		break;
	}

	return QVariant();
}

QVariant NotesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
	{
		switch(section)
		{
			case 0:
			return tr("Subject");

			case 1:
			return tr("Sender");

			case 2:
			return tr("Date");

			default:
			break;
		}
	}

	return QAbstractTableModel::headerData(section, orientation, role);
}

bool NotesModel::canFetchMore(const QModelIndex &parent) const
{
	return !m_waitingData && (m_folder.maxOffset > 0) && (m_folder.maxOffset > m_folder.notes.size());
}

void NotesModel::fetchMore(const QModelIndex &parent)
{
	m_waitingData = true;

	emit loadNewData(m_folder.notes.size());
}
