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
#include "settingsdialog.h"
#include "moc_settingsdialog.cpp"
#include "configfile.h"
#include "utils.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

SettingsDialog::SettingsDialog(QWidget* parent):QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint)
{
	setupUi(this);

	QStringList filter("*.css");

	QStringList styles;
	styles << QDir(QString("%1/styles").arg(ConfigFile::getInstance()->getLocalDataDirectory())).entryList(filter);
	styles << QDir(QString("%1/styles").arg(ConfigFile::getInstance()->getGlobalDataDirectory())).entryList(filter);

	styles.replaceInStrings(".css", "");
	styles.removeDuplicates();
	styles.sort();
	styles.prepend(QString(tr("<default>")));

	int logIndex = 0, screenIndex = 0;

	QString screenStyle = ConfigFile::getInstance()->getScreenStyle();
	QString logStyle = ConfigFile::getInstance()->getLogStyle();

	for(int i = 0; i < styles.size(); ++i)
	{
		if (styles[i] == screenStyle) screenIndex = i;
		if (styles[i] == logStyle) logIndex = i;
	}

	QStringListModel *model = new QStringListModel(styles, this);

	// chat
	displayTimestampsCheckBox->setChecked(ConfigFile::getInstance()->getDisplayTimestamps());
	useSystemTrayCheckBox->setChecked(ConfigFile::getInstance()->getUseSystray());
	hideWindowCheckBox->setChecked(ConfigFile::getInstance()->getHideMinimizedWindow());

	highlightColorButton->setUserData(0, new StringUserData(ConfigFile::getInstance()->getHighlightColor().name()));
	errorColorButton->setUserData(0, new StringUserData(ConfigFile::getInstance()->getErrorColor().name()));
	chatStyleComboBox->setModel(model);
	chatStyleComboBox->setCurrentIndex(screenIndex);

	updateButtonsColors();

	connect(highlightColorButton, SIGNAL(clicked()), this, SLOT(onHighlightColorClicked()));
	connect(errorColorButton, SIGNAL(clicked()), this, SLOT(onErrorColorClicked()));

	// logs
	logsGroupBox->setChecked(ConfigFile::getInstance()->getEnableLogs());
	enableLogsTextCheckBox->setChecked(ConfigFile::getInstance()->getEnableTextLogs());
	enableLogsHTMLCheckBox->setChecked(ConfigFile::getInstance()->getEnableHtmlLogs());
	logsDirectoryEdit->setText(ConfigFile::getInstance()->getLogsDirectory());
	logsStyleComboBox->setModel(model);
	logsStyleComboBox->setCurrentIndex(logIndex);

	connect(logsBrowseButton, SIGNAL(clicked()), this, SLOT(onLogsBrowseClicked()));

	// animations
	animationRefreshSpinBox->setValue(ConfigFile::getInstance()->getAnimationFrameDelay());
	enableAnimationsGroupBox->setChecked(ConfigFile::getInstance()->getEnableAnimations());

	// sounds
	enableSoundGroupBox->setChecked(ConfigFile::getInstance()->getEnableSound());
	nameMentionedSoundEdit->setText(ConfigFile::getInstance()->getNameMentionedSound());
	noteReceivedSoundEdit->setText(ConfigFile::getInstance()->getNoteReceivedSound());

	connect(nameMentionedSoundBrowseButton, SIGNAL(clicked()), this, SLOT(onNameMentionedSoundBrowseClicked()));
	connect(noteReceivedSoundBrowseButton, SIGNAL(clicked()), this, SLOT(onNoteReceivedSoundBrowseClicked()));
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::accept()
{
	ConfigFile *config = ConfigFile::getInstance();

	// chat
	config->setHighlightColor(((StringUserData*)highlightColorButton->userData(0))->text);
	config->setErrorColor(((StringUserData*)errorColorButton->userData(0))->text);
	config->setScreenStyle(chatStyleComboBox->currentIndex() == 0 ? "":chatStyleComboBox->currentText());
	config->setDisplayTimestamps(displayTimestampsCheckBox->isChecked());
	ConfigFile::getInstance()->setUseSystray(useSystemTrayCheckBox->isChecked());
	ConfigFile::getInstance()->setHideMinimizedWindow(hideWindowCheckBox->isChecked());

	// logs
	config->setEnableLogs(logsGroupBox->isChecked());
	config->setEnableTextLogs(enableLogsTextCheckBox->isChecked());
	config->setEnableHtmlLogs(enableLogsHTMLCheckBox->isChecked());
	config->setLogsDirectory(logsDirectoryEdit->text());
	config->setLogStyle(logsStyleComboBox->currentIndex() == 0 ? "":logsStyleComboBox->currentText());

	// animations
	config->setAnimationFrameDelay(animationRefreshSpinBox->value());
	config->setEnableAnimations(enableAnimationsGroupBox->isChecked());

	// sounds
	config->setEnableSound(enableSoundGroupBox->isChecked());
	config->setNameMentionedSound(nameMentionedSoundEdit->text());
	config->setNoteReceivedSound(noteReceivedSoundEdit->text());

	// fix settings with wrong values
	config->updateSettings();

	QDialog::accept();
}

void SettingsDialog::onHighlightColorClicked()
{
	QColor defaultColor(((StringUserData*)highlightColorButton->userData(0))->text);
	QColor color = QColorDialog::getColor(defaultColor, this);

	if (color.isValid())
	{
		highlightColorButton->setUserData(0, new StringUserData(color.name()));
		updateButtonsColors();
	}
}

void SettingsDialog::onErrorColorClicked()
{
	QColor defaultColor(((StringUserData*)errorColorButton->userData(0))->text);
	QColor color = QColorDialog::getColor(defaultColor, this);

	if (color.isValid())
	{
		errorColorButton->setUserData(0, new StringUserData(color.name()));
		updateButtonsColors();
	}
}

void SettingsDialog::onLogsBrowseClicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Choose logs directory"), logsDirectoryEdit->text());

	if (!dir.isEmpty())
	{
		logsDirectoryEdit->setText(dir);
	}
}

void SettingsDialog::onNameMentionedSoundBrowseClicked()
{
	QString dir = QFileDialog::getOpenFileName(this, tr("Choose WAV file"), nameMentionedSoundEdit->text(), tr("Audio files (*.wav)"));

	if (!dir.isEmpty())
	{
		nameMentionedSoundEdit->setText(dir);
	}
}

void SettingsDialog::onNoteReceivedSoundBrowseClicked()
{
	QString dir = QFileDialog::getOpenFileName(this, tr("Choose WAV file"), noteReceivedSoundEdit->text(), tr("Audio files (*.wav)"));

	if (!dir.isEmpty())
	{
		noteReceivedSoundEdit->setText(dir);
	}
}

void SettingsDialog::updateButtonsColors()
{
	QColor highlightColor(((StringUserData*)highlightColorButton->userData(0))->text);

	highlightColorButton->setStyleSheet(QString("background-color: %1;").arg(highlightColor.name()));

	QColor errorColor(((StringUserData*)errorColorButton->userData(0))->text);

	errorColorButton->setStyleSheet(QString("background-color: %1;").arg(errorColor.name()));
}
