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

#ifndef NOTESFRAME_H
#define NOTESFRAME_H

#include "ui_notesframe.h"

#include "tabframe.h"
#include "notesmodel.h"

class NotesSortFilterProxyModel;

class NotesFrame : public TabFrame, public Ui::NotesFrame
{
	Q_OBJECT

public:
	NotesFrame(QWidget *parent);
	virtual ~NotesFrame();

	virtual void setSystem(const QString &text);

	QString getCurrentFolderId() const;

	void setFolder(const Folder &folder);
	void updateFolder(const Folder &folder, int offset, int count);
	void updateNote(const Note &note);

public slots:
	void onSearchChanged(const QString &search);
	void onSearch();
	void onLoadNewData(int offset);
	void onNotesSelected(const QItemSelection &selected, const QItemSelection &deselected);

protected:
	void resizeEvent(QResizeEvent *e);

	NotesModel *m_model;
	NotesSortFilterProxyModel *m_proxyModel;

	QString m_folderId;
};

#endif
