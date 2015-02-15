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
#include "updatedialog.h"
#include "moc_updatedialog.cpp"
#include "configfile.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

UpdateDialog::UpdateDialog(QWidget* parent):QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint), m_manager(NULL), m_reply(NULL), m_size(0)
{
	setupUi(this);

	installButton->setVisible(false);
	retryButton->setVisible(false);

	layout()->setSizeConstraint(QLayout::SetFixedSize);

	m_manager = new QNetworkAccessManager(this);

	connect(m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onReply(QNetworkReply*)));

	connect(installButton, SIGNAL(clicked()), SLOT(onInstall()));
	connect(cancelButton, SIGNAL(clicked()), SLOT(onCancel()));
	connect(retryButton, SIGNAL(clicked()), SLOT(onRetry()));
}

UpdateDialog::~UpdateDialog()
{
}

bool UpdateDialog::download(const QString &url, uint size)
{
	m_size = size;
	m_url = url;
#ifdef USE_QT5
	m_filename = QUrl(url).fileName();
#else
	m_filename = QFileInfo(QUrl(url).path()).fileName();
#endif
	m_fullpath = getOutputFilename(m_filename);

	return download();
}

bool UpdateDialog::download()
{
	m_reply = m_manager->get(QNetworkRequest(QUrl(m_url)));

	connect(m_reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(onDownloadProgress(qint64, qint64)));

	// update label
	label->setText(tr("Downloading %1...").arg(m_filename));

	// update progress bar
	progressBar->setRange(0, m_size);

	// update Windows progress bar
	emit downloadProgress(0, m_size);

	return true;
}

void UpdateDialog::onInstall()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(m_fullpath));

	accept();
}

void UpdateDialog::onCancel()
{
	// abort download if any
	if (m_reply) m_reply->abort();

	close();
}

void UpdateDialog::onRetry()
{
	download();
}

void UpdateDialog::onDownloadProgress(qint64 readBytes, qint64 totalBytes)
{
	// update progress bar
	progressBar->setValue(readBytes);

	// update Windows progress bar
	emit downloadProgress(readBytes, totalBytes);
}

void UpdateDialog::onReply(QNetworkReply *reply)
{
	m_reply = NULL;

	if (reply->error() != QNetworkReply::NoError)
	{
		label->setText(tr("An error occurred when downloading, please click on \"Retry\" to restart download."));
		retryButton->setVisible(true);
		reply->deleteLater();
		return;
	}

	QByteArray content = reply->readAll();

	reply->deleteLater();

	QFile file(m_fullpath);

	if (file.open(QIODevice::WriteOnly))
	{
		file.write(content);

		label->setText(tr("Your download is complete, click on \"Install\" to install the new version."));
		installButton->setVisible(true);
	}
	else
	{
		label->setText(tr("Your download is complete, but we're unable to create file %1.").arg(m_fullpath));
	}
}

QString UpdateDialog::getOutputFilename(const QString &filename)
{
	QString res;

	int i = 0;

	QFileInfo info(filename);
	QString path = ConfigFile::getInstance()->getDownloadDirectory();

	do
	{
		if (i == 0)
		{
			res = QString("%1/%2.%3").arg(path).arg(info.completeBaseName()).arg(info.suffix());
		}
		else
		{
			res = QString("%1/%2 (%3).%4").arg(path).arg(info.completeBaseName()).arg(i).arg(info.suffix());
		}

		++i;
	}
	while(QFile::exists(res));

	return res;
}
