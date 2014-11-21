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

#include <QtCore/QAbstractItemModel>

struct Note
{
	QString id;
	QString folderId;
	QString subject;
	QString preview; // without cariage returns
	QString date; // Oct 30, 2014, 1:50:33 PM
	QString sender;
	QStringList recipients;
	QString text;
	QString html;
	bool hasSignature;

	Note():hasSignature(false)
	{
	}

	bool operator == (const Note &other) const
	{
		return id == other.id;
	}
};

typedef QVector<Note> Notes;

struct Folder
{
	QString id;
	QString name;
	Notes notes;
	int count; // notes per request
	int offset; // current offset
	int maxOffset; // last offset
};

typedef QMap<QString, Folder> Folders;

class NotesModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	NotesModel(QObject *parent);
	virtual ~NotesModel();

	void setNotes(const Notes &notes);
	void updateNotes(const Notes &notes, int offset, int count);

	QModelIndex index(int row, int column, const QModelIndex &parent) const;
	QModelIndex parent(const QModelIndex &index) const;
	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

protected:
	Notes m_notes;
};

#endif
