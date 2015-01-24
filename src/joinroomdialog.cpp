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
#include "joinroomdialog.h"
#include "configfile.h"
#include "moc_joinroomdialog.cpp"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

JoinRoomDialog::JoinRoomDialog(QWidget* parent):QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint)
{
	setupUi(this);

	QStringListModel *model = new QStringListModel(this);
	roomsView->setModel(model);

	// TODO: remove rooms already joined
	QStringList list;

	ConfigRooms rooms = ConfigFile::getInstance()->getRooms();

	ConfigRoomsIterator it = rooms.begin();

	while(it != rooms.end())
	{
		if (!it->connected) list << it->name;

		++it;
	}

	model->setStringList(list);

	connect(roomsView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), this, SLOT(onRoom(QModelIndex)));
}

JoinRoomDialog::~JoinRoomDialog()
{
}

QString JoinRoomDialog::getRoom() const
{
	return roomEdit->text();
}

void JoinRoomDialog::onRoom(const QModelIndex &index)
{
	QString text = roomsView->model()->data(index).toString();

	roomEdit->setText(text);
}

