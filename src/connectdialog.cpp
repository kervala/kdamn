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
#include "connectdialog.h"
#include "configfile.h"
#include "moc_connectdialog.cpp"

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

ConnectDialog::ConnectDialog(QWidget* parent):QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint)
{
	setupUi(this);

	loginEdit->setText(ConfigFile::getInstance()->getLogin());
	passwordEdit->setText(ConfigFile::getInstance()->getPassword());
	tokenEdit->setText(ConfigFile::getInstance()->getDAmnToken());
	rememberPasswordBox->setChecked(ConfigFile::getInstance()->isRememberPassword());
}

ConnectDialog::~ConnectDialog()
{
}
