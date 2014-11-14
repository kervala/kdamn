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

#ifndef FOLDERFRAME_H
#define FOLDERFRAME_H

#include "ui_folderframe.h"

#include "tabframe.h"
#include "notesmodel.h"

class FolderFrame : public TabFrame, public Ui::FolderFrame
{
	Q_OBJECT

public:
	FolderFrame(QWidget *parent);
	virtual ~FolderFrame();

	virtual void setSystem(const QString &text);

	void setNotes(const Notes &notes);

public slots:
	void onSearch();

protected:
	void updateSplitter();

	NotesModel *m_model;
};

#endif
