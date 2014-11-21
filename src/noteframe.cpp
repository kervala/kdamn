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
#include "noteframe.h"
#include "oauth2.h"
#include "moc_noteframe.cpp"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

NoteFrame::NoteFrame(QWidget *parent):TabFrame(parent)
{
	setupUi(this);

	connect(draftButton, SIGNAL(clicked()), SLOT(onSaveDraft()));
	connect(previewButton, SIGNAL(clicked()), SLOT(onPreview()));
	connect(sendButton, SIGNAL(clicked()), SLOT(onSend()));
//	connect(usersView, SIGNAL(doubleClicked(QModelIndex)), SLOT(onUserDoubleClicked(QModelIndex)));
}

NoteFrame::~NoteFrame()
{
}

void NoteFrame::setSystem(const QString &text)
{
}

void NoteFrame::onSaveDraft()
{
}

void NoteFrame::onPreview()
{
	QString body = bodyTextEdit->toPlainText();
	bool hasSignature = signatureBox->isChecked();

	OAuth2::getInstance()->requestNotesPreview(body, hasSignature);
}

void NoteFrame::onSend()
{
	// format note
	Note note;
	note.recipients = toEdit->text().split(",");
	note.subject = subjectEdit->text();
	note.text = bodyTextEdit->toPlainText();
	note.hasSignature = signatureBox->isChecked();

	// send note
	if (OAuth2::getInstance()->sendNote(note))
	{
		// if sending is ok, disable form
		setDisabled(true);
	}
}
