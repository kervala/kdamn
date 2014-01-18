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
#include "capturedialog.h"
#include "utils.h"

CaptureDialog::CaptureDialog(QWidget *parent):QDialog(parent), m_handle(0)
{
	setupUi(this);

	// disable OK button
	buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	QStandardItemModel *model = new QStandardItemModel(0, 1, this);

	// add windows list with icons to listview model
	CreateWindowsList(model);

	windowsListView->setModel(model);

	int rowHeight = windowsListView->sizeHintForRow(0) + 1;
	windowsListView->setMinimumHeight(std::min(6, model->rowCount()) * rowHeight);

	connect(windowsListView, SIGNAL(pressed(const QModelIndex &)), this, SLOT(enableButton(const QModelIndex &)));
	connect(windowsListView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(validateButton(const QModelIndex &)));
}

void CaptureDialog::enableButton(const QModelIndex &index)
{
	buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

	QVariant val = windowsListView->model()->data(index, Qt::UserRole);

#ifdef Q_OS_WIN
	m_handle = (WId)val.value<void*>();
#else
	m_handle = (WId)val.value<WId>();
#endif

	m_name = windowsListView->model()->data(index).toString();
}

void CaptureDialog::validateButton(const QModelIndex &index)
{
	enableButton(index);

	accept();
}