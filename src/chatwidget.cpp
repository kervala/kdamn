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
#include "chatwidget.h"
#include "roomframe.h"
#include "oauth2.h"
#include "configfile.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

QString ChatWidget::s_css = \
	".timestamp { color: #999; }\n" \
	".username { font-weight: bold; }\n" \
	".error { color: #f00; }\n" \
	".system { color: #666; }\n" \
	".hidden { display: none; }\n";

ChatWidget::ChatWidget(QWidget *parent):QTextBrowser(parent), m_focus(false), m_lastReload(QDateTime::currentDateTime())
{
	connect(this, SIGNAL(anchorClicked(QUrl)), this, SLOT(onUrl(QUrl)));

	document()->setDefaultStyleSheet(s_css);

	setAcceptDrops(true);
}

ChatWidget::~ChatWidget()
{
	closeLogs();
}

void ChatWidget::setAction(const QString &user, const QString &text, bool html)
{
	if (html)
	{
		appendHtml(QString("<div class=\"normal\">%1<span class=\"username\">%2</span> %3</div>").arg(getTimestamp(true)).arg(user).arg(text));

		startAnimations(text);
	}
	else
	{
		appendText(QString("%1%2 %3").arg(getTimestamp(false)).arg(user).arg(text));
	}
}

void ChatWidget::setText(const QString &user, const QString &text, bool html)
{
	if (html)
	{
		appendHtml(QString("<div class=\"normal\">%1<span class=\"username\">&lt;%2&gt;</span> %3</div>").arg(getTimestamp(true)).arg(user).arg(text));

		startAnimations(text);
	}
	else
	{
		appendText(QString("%1<%2> %3").arg(getTimestamp(false)).arg(user).arg(text));
	}
}

void ChatWidget::setSystem(const QString &text, bool html)
{
	if (html)
	{
		appendHtml(QString("<div class=\"system\">%1%2</div>").arg(getTimestamp(true)).arg(text));

		startAnimations(text);
	}
	else
	{
		appendText(QString("%1%2").arg(getTimestamp(false)).arg(text));
	}
}

void ChatWidget::setError(const QString &error)
{
	appendHtml(QString("<div class=\"error\">%1%2</div>").arg(getTimestamp(true)).arg(error));
	appendText(QString("%1%2").arg(getTimestamp(false)).arg(error));
}

void ChatWidget::onUrl(const QUrl &url)
{
	// open URL in default browser
	QDesktopServices::openUrl(url);
}

void ChatWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void ChatWidget::dragMoveEvent(QDragMoveEvent *event)
{
	if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void ChatWidget::dropEvent(QDropEvent *event)
{
	const QMimeData *mimeData = event->mimeData();

	if (mimeData->hasUrls())
	{
		foreach(const QUrl &url, mimeData->urls())
		{
			if (!OAuth2::getInstance()->uploadToStash(url.toLocalFile(), m_room)) break;
		}

		event->acceptProposedAction();
	}
}

void ChatWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

QString ChatWidget::getTimestamp(bool html) const
{
	// check if user doesn't want to display timestamp
	if (!ConfigFile::getInstance()->getDisplayTimestamp()) return QString();

	QString timestamp = QTime::currentTime().toString();

	return QString(html ? "<span class=\"timestamp\">%1</span> ":"[%1] ").arg(timestamp);
}

void ChatWidget::appendHtml(const QString &html)
{
	// reset previous formatting, to cancel wrong behavior from previous HTML tags
	if (!textCursor().blockFormat().properties().isEmpty())
	{
		// create a new empty block to keep previous forrmatting
		append("<div class=\"hidden\" />");

		// reset block formatting
		textCursor().setBlockFormat(QTextBlockFormat());
	}

	append(html);

	if (m_htmlFile.isOpen()) m_htmlFile.write(html.toUtf8() + "\n");
}

void ChatWidget::appendText(const QString &text)
{
	if (m_textFile.isOpen()) m_textFile.write(text.toUtf8() + "\n");
}

QVariant ChatWidget::loadResource(int type, const QUrl &name)
{
	// return an empty variant to not trigger an error
	return QVariant(QImage(":/images/blank.png"));
}

bool ChatWidget::alreadyLoaded(const QString &url) const
{
	QHash<QMovie*, QUrl>::const_iterator it = m_urls.constBegin();

	while(it != m_urls.end())
	{
		if (it->toString() == url) return true;

		++it;
	}

	return false;
}

void ChatWidget::startAnimations(const QString &html)
{
	// parse HTML code to find local images
	QRegExp reg("src=\"(https?://([^\"]+))\" local=\"(file://([^\"]+))\"");

	int pos = 0;

	while((pos = reg.indexIn(html, pos)) != -1)
	{
		// found an URL
		QString url = reg.cap(1);
		QString file = reg.cap(3);

		if (!alreadyLoaded(url)) new AnimationStart(url, file, this);

		pos += reg.matchedLength();
	}
}

bool ChatWidget::addAnimation(const QString& url, const QString &file)
{
	if (alreadyLoaded(url)) return true;

	QUrl remoteUrl(url);
	QUrl localUrl(file);

	QMovie* movie = new QMovie(this);

	movie->setFileName(localUrl.toLocalFile());

	if (!movie->isValid())
	{
		delete movie;
		new AnimationStart(url, file, this);

		return false;
	}

	QString test = movie->format();

/*
	if (movie->frameCount() < 2)
	{
		return false;
	}
	else
	{
	}
*/

	m_urls.insert(movie, remoteUrl);

	connect(movie, SIGNAL(frameChanged(int)), this, SLOT(animate(int)));

	// start only when in foreground
	if (m_focus) movie->start();

	return true;
}

void ChatWidget::setFocus(bool focus)
{
	m_focus = focus;

	QHash<QMovie*, QUrl>::iterator it = m_urls.begin();

	while(it != m_urls.end())
	{
		if (m_focus)
		{
			it.key()->start();
		}
		else
		{
			it.key()->stop();
		}

		++it;
	}
}

void ChatWidget::setRoom(const QString &room)
{
	m_room = room;

	openLogs();
}

void ChatWidget::openLogs()
{
	QString cachePath;

#ifdef USE_QT5
	cachePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#else
	cachePath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#endif

	QString dir = QString("%1/kdAmn/logs").arg(QDir::fromNativeSeparators(cachePath));

	if (!QDir().mkpath(dir))
	{
		qDebug() << "Unable to create directory" << dir;

		return;
	}

	QString filename = QString("%1/%2-%3").arg(dir).arg(m_room).arg(QDate::currentDate().toString(Qt::ISODate));

	m_htmlFile.setFileName(filename + ".htm");

	if (m_htmlFile.open(QIODevice::Append))
	{
		QString header;
		header += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		header += "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n";
		header += "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n";
		header += "<head>\n";
		header += "<style type=\"text/css\">\n";
		header += s_css;
		header += "</style>\n";
		header += "</head>\n";
		header += "<body>\n";

		m_htmlFile.write(header.toUtf8());
	}
	else
	{
		qDebug() << "Unable to open file" << (filename + ".htm");
	}

	m_textFile.setFileName(filename + ".txt");

	if (m_textFile.open(QIODevice::Append))
	{
	}
	else
	{
		qDebug() << "Unable to open file" << (filename + ".txt");
	}
}

void ChatWidget::closeLogs()
{
	if (m_htmlFile.isOpen())
	{
		m_htmlFile.write("</body></html>");
		m_htmlFile.close();
	}

	m_textFile.close();
}

void ChatWidget::animate(int frame)
{
	if (QMovie* movie = qobject_cast<QMovie*>(sender()))
	{
		document()->addResource(QTextDocument::ImageResource, m_urls.value(movie), movie->currentPixmap());

		QDateTime current = QDateTime::currentDateTime();

		// to improve performances, only reload page at regular interval
		if (m_lastReload.msecsTo(current) > ConfigFile::getInstance()->getAnimationFrameDelay())
		{
			m_lastReload = current;

#ifdef USE_QT5
			QRectF r(rect());
			r.translate(0, verticalScrollBar()->value());
			document()->documentLayout()->update(r);
#else
			viewport()->update(rect());
#endif
		}
	}
}

AnimationStart::AnimationStart(const QString &url, const QString &file, ChatWidget *widget):QObject(widget), m_url(url), m_file(file), m_widget(widget)
{
	int delayAnimation = ConfigFile::getInstance()->getAnimationFrameDelay();

	QTimer::singleShot(delayAnimation, this, SLOT(timeout()));
}

void AnimationStart::timeout()
{
	m_widget->addAnimation(m_url, m_file);
}
