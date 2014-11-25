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

#ifndef FOLDER_H
#define FOLDER_H

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
	bool starred;
	bool unread;
	bool replied;

	Note():hasSignature(false), starred(false), unread(false), replied(false)
	{
	}

	bool operator == (const Note &other) const
	{
		return id == other.id;
	}
};

typedef QVector<Note> Notes;

class Folder
{
public:
	Folder();

	bool updateValues(int count, int maxOffset);

	bool load();
	bool save();

	bool addNote(const Note &note);
	bool exportNotes(const QString &dir);

	QString id;
	QString name;
	Notes notes;
	int count; // notes per request
//	int offset; // current offset
	int maxOffset; // last offset
};

typedef QMap<QString, Folder> Folders;

#endif
