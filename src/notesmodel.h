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

#ifndef NOTESMODEL_H
#define NOTESMODEL_H

#include "folder.h"

#include <QtCore/QAbstractTableModel>

class NotesModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	NotesModel(QObject *parent);
	virtual ~NotesModel();

	void setFolder(const Folder &folder);
	void updateFolder(const Folder &folder, int offset, int count);

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	bool canFetchMore (const QModelIndex &parent) const;
	void fetchMore (const QModelIndex &parent);

signals:
	void loadNewData(int offset) const;

protected:
	Folder m_folder;
	bool m_waitingData;
};

#endif
