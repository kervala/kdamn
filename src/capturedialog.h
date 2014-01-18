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

#ifndef CAPTUREDIALOG_H
#define CAPTUREDIALOG_H

#include "ui_capturedialog.h"

class CaptureDialog : public QDialog, public Ui::CaptureDialog
{
	Q_OBJECT

public:
	CaptureDialog(QWidget *parent);

	WId getWindowId() const { return m_handle; }
	const QString& getWindowName() const { return m_name; }

public slots:
	void enableButton(const QModelIndex &index);
	void validateButton(const QModelIndex &index);

private:
	WId m_handle;
	QString m_name;
};

#endif
