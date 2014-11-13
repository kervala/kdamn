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

#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include "ui_updatedialog.h"

class UpdateDialog : public QDialog, public Ui::UpdateDialog
{
	Q_OBJECT

public:
	UpdateDialog(QWidget* parent);
	virtual ~UpdateDialog();

	bool download(const QString &url, uint size);

signals:
	void downloadProgress(qint64 readBytes, qint64 totalBytes);

public slots:
	// buttons
	void onInstall();
	void onCancel();
	void onRetry();

	// reply slots
	void onDownloadProgress(qint64 readBytes, qint64 totalBytes);
	void onReply(QNetworkReply *reply);

private:
	bool download();
	QString getOutputFilename(const QString &filename);

	QNetworkAccessManager *m_manager;
	QNetworkReply *m_reply;

	uint m_size;
	QString m_url;
	QString m_filename;
	QString m_fullpath;
};

#endif
