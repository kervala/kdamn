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
#include "notesframe.h"
#include "moc_notesframe.cpp"
#include "notessortfilterproxymodel.h"
#include "oauth2.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

NotesFrame::NotesFrame(QWidget *parent):TabFrame(parent), m_model(NULL)
{
	setupUi(this);

	m_model = new NotesModel(this);

	m_proxyModel = new NotesSortFilterProxyModel(this);
	m_proxyModel->setDynamicSortFilter(true);
	m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	m_proxyModel->setSourceModel(m_model);

	notesView->setModel(m_proxyModel);

	// by default sort by date descending order
	m_proxyModel->sort(2, Qt::DescendingOrder);
	notesView->horizontalHeader()->setSortIndicator(2, Qt::DescendingOrder);

#ifdef USE_QT5
	notesView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
#else
	// TODO: Qt 4 equivalent
#endif
	notesView->verticalHeader()->setDefaultSectionSize(notesView->fontMetrics().height() + 4);

	notesView->horizontalHeader()->setCascadingSectionResizes(true);
	notesView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
	notesView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
	notesView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);

	int dateWidth = notesView->fontMetrics().width("0000-00-00 00:00:00") + 4;
	notesView->horizontalHeader()->resizeSection(1, 150);
	notesView->horizontalHeader()->resizeSection(2, dateWidth);
	// we'll compute first section width later

	connect(searchEdit, SIGNAL(textChanged(QString)), this, SLOT(onSearchChanged(QString)));
	connect(searchEdit, SIGNAL(returnPressed()), SLOT(onSearch()));
//	connect(usersView, SIGNAL(doubleClicked(QModelIndex)), SLOT(onUserDoubleClicked(QModelIndex)));
	connect(m_model, SIGNAL(loadNewData(int)), SLOT(onLoadNewData(int)));

	connect(notesView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), SLOT(onNotesSelected(QItemSelection, QItemSelection)));
}

NotesFrame::~NotesFrame()
{
}

void NotesFrame::setSystem(const QString &text)
{
}

QString NotesFrame::getCurrentFolderId() const
{
	return m_folderId;
}

void NotesFrame::setFolder(const Folder &folder)
{
	if (!folder.notes.isEmpty() && m_folderId.isEmpty()) m_folderId = folder.notes.front().folderId;

	m_model->setFolder(folder);
}

void NotesFrame::updateFolder(const Folder &folder, int offset, int count)
{
	if (!folder.notes.isEmpty() && m_folderId.isEmpty()) m_folderId = folder.notes.front().folderId;

	m_model->updateFolder(folder, offset, count);
}

void NotesFrame::updateNote(const Note &note)
{
	m_model->updateNote(note);

	// TODO: check if this is the selected note

	previewEdit->setText(note.text);
}

void NotesFrame::onSearchChanged(const QString &search)
{
	m_proxyModel->setFilterFixedString(search);
}

void NotesFrame::onSearch()
{
//	QStringList lines = searchEdit->getLines();
}

void NotesFrame::onLoadNewData(int offset)
{
	OAuth2::getInstance()->requestNotesDisplayFolder(m_folderId, offset);
}

void NotesFrame::resizeEvent(QResizeEvent *e)
{
	int totalWidth = e->size().width() - qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent) - 4;
	int senderWidth = notesView->horizontalHeader()->sectionSize(1);
	int dateWidth = notesView->horizontalHeader()->sectionSize(2);

	notesView->horizontalHeader()->resizeSection(0, totalWidth - senderWidth - dateWidth);
	notesView->horizontalHeader()->resizeSection(1, senderWidth);
	notesView->horizontalHeader()->resizeSection(2, dateWidth);

	TabFrame::resizeEvent(e);
}

void NotesFrame::onNotesSelected(const QItemSelection &selected, const QItemSelection &deselected)
{
	QString preview;
	QString noteId;

	foreach(const QItemSelectionRange &range, selected)
	{
		int top = m_proxyModel->mapToSource(m_proxyModel->index(range.top(), 0)).row();
		int bottom = m_proxyModel->mapToSource(m_proxyModel->index(range.bottom(), 0)).row();

		for (int i = top; i <= bottom; ++i)
		{
			const Note &note = m_model->getNote(i);

			if (note.text.isEmpty())
			{
				preview = note.preview;
				noteId = note.id;
			}
			else
			{
				preview = note.text;
			}

			break;
		}
	}

	previewEdit->setText(preview);

	if (!noteId.isEmpty()) OAuth2::getInstance()->requestNotesDisplayNote(m_folderId, noteId);
}
