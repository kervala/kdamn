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

#ifndef JOINCHANNELDIALOG_H
#define JOINCHANNELDIALOG_H

#include "ui_joinchanneldialog.h"

class JoinChannelDialog : public QDialog, public Ui::JoinChannelDialog
{
	Q_OBJECT

public:
	JoinChannelDialog(QWidget* parent);
	virtual ~JoinChannelDialog();

	QString getChannel() const;

public slots:
	void onChannel(const QModelIndex &index);
};

#endif
