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

class NotesFrame : public TabFrame, public Ui::NotesFrame
{
	Q_OBJECT

public:
	NotesFrame(QWidget *parent);
	virtual ~NotesFrame();

	virtual void setSystem(const QString &text);

	QString getCurrentFolderId() const;

	void setNotes(const Notes &notes);
	void updateNotes(const Notes &notes, int offset, int count);

public slots:
	void onSearch();

protected:
	void updateSplitter();

	NotesModel *m_model;

	QString m_folderId;
};

#endif
