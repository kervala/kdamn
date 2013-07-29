/*
 *  kdAmn is a deviantART Messaging Network client
 *  Copyright (C) 2013  Cedric OCHS
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
#include "joinchanneldialog.h"
#include "configfile.h"
#include "moc_joinchanneldialog.cpp"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

JoinChannelDialog::JoinChannelDialog(QWidget* parent):QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint)
{
	setupUi(this);

	QStringListModel *model = new QStringListModel(this);
	channelsView->setModel(model);

	// TODO: remove channels already joined
	QStringList list;

	ConfigChannels channels = ConfigFile::getInstance()->getChannels();

	ConfigChannelsIterator it = channels.begin();

	while(it != channels.end())
	{
		if (!it->connected) list << it->name;

		++it;
	}

	model->setStringList(list);

	connect(channelsView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), this, SLOT(onChannel(QModelIndex)));
}

JoinChannelDialog::~JoinChannelDialog()
{
}

QString JoinChannelDialog::getChannel() const
{
	return channelEdit->text();
}

void JoinChannelDialog::onChannel(const QModelIndex &index)
{
	QString text = channelsView->model()->data(index).toString();

	channelEdit->setText(text);
}
