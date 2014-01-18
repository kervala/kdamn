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
#include "settingsdialog.h"
#include "moc_settingsdialog.cpp"
#include "configfile.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

SettingsDialog::SettingsDialog(QWidget* parent):QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint)
{
	setupUi(this);

	animationRefreshSpinBox->setValue(ConfigFile::getInstance()->getAnimationFrameDelay());
	displayTimestampsCheckBox->setChecked(ConfigFile::getInstance()->getDisplayTimestamps());
	enableAnimationsGroupBox->setChecked(ConfigFile::getInstance()->getEnableAnimations());
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::accept()
{
	ConfigFile::getInstance()->setAnimationFrameDelay(animationRefreshSpinBox->value());
	ConfigFile::getInstance()->setDisplayTimestamps(displayTimestampsCheckBox->isChecked());
	ConfigFile::getInstance()->setEnableAnimations(enableAnimationsGroupBox->isChecked());

	QDialog::accept();
}