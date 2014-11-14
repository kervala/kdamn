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
#include "folderframe.h"
#include "moc_folderframe.cpp"
#include "notesmodel.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

FolderFrame::FolderFrame(QWidget *parent):TabFrame(parent), m_model(NULL)
{
	setupUi(this);

	m_model = new NotesModel(this);

	notesView->setModel(m_model);

	notesView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	notesView->verticalHeader()->setDefaultSectionSize(notesView->fontMetrics().height() + 4);

	connect(searchEdit, SIGNAL(returnPressed()), SLOT(onSearch()));
//	connect(usersView, SIGNAL(doubleClicked(QModelIndex)), SLOT(onUserDoubleClicked(QModelIndex)));
}

FolderFrame::~FolderFrame()
{
}

void FolderFrame::setSystem(const QString &text)
{
}

void FolderFrame::setNotes(const Notes &notes)
{
	m_model->setNotes(notes);

	notesView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
}

void FolderFrame::onSearch()
{
//	QStringList lines = searchEdit->getLines();
}

void FolderFrame::updateSplitter()
{
/*
	QStringList users = m_usersModel->stringList();

	int min = 65536;
	int max = 0;

	foreach(const QString &user, users)
	{
//		int width = usersView->fontMetrics().boundingRect(member.name).width();
		int width = usersView->fontMetrics().width(user)+10;

		if (width < min) min = width;
		if (width > max) max = width;
	}

	usersView->setMinimumWidth(min);
	usersView->setMaximumWidth(max);
*/
}
