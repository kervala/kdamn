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
#include "chatwidget.h"
#include "moc_chatwidget.cpp"
#include "oauth2.h"
#include "configfile.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

ChatWidget::ChatWidget(QWidget *parent):QTextBrowser(parent), m_focus(false), m_lastReload(QDateTime::currentDateTime())
{
	connect(this, SIGNAL(anchorClicked(QUrl)), this, SLOT(onUrl(QUrl)));

	setAcceptDrops(true);
}

ChatWidget::~ChatWidget()
{
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
		QStringList filenames;

		foreach(const QUrl &url, mimeData->urls())
		{
			filenames << url.toLocalFile();
		}

		OAuth2::getInstance()->uploadToStash(filenames, m_room);

		event->acceptProposedAction();
	}
}

void ChatWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

void ChatWidget::appendHtml(const QString &html)
{
	QMutexLocker lock(&m_contentMutex);

	// reset previous formatting, to cancel wrong behavior from previous HTML tags
	if (!textCursor().blockFormat().properties().isEmpty())
	{
		// create a new empty block to keep previous formatting
		append("<div class=\"hidden\" />");

		// reset block formatting
		textCursor().setBlockFormat(QTextBlockFormat());
	}

	m_lines << html;

	append(html);

	startAnimations(html);
}

void ChatWidget::updateCss(const QString &css)
{
	document()->setDefaultStyleSheet(css + ".hidden { display: none; }\n");

	clear();

	QColor bodyColor = QApplication::palette().base().color();

	// parse background-color
	QRegExp bodyColorReg("body \\{ background-color: ([^;]+); \\}");

	if (bodyColorReg.indexIn(css) > -1)
	{
		bodyColor = QColor(bodyColorReg.cap(1));
	}

	QPalette p = palette();
	p.setColor(QPalette::Base, bodyColor);
	setPalette(p);

	foreach(const QString &line, m_lines)
	{
		append(line);
	}
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

	m_urls.insert(movie, remoteUrl);

	connect(movie, SIGNAL(frameChanged(int)), this, SLOT(animate(int)));

	// always start animation (we want at least the 1st frame)
	movie->start();

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
}

void ChatWidget::keyPressEvent(QKeyEvent *e)
{
	QTextBrowser::keyPressEvent(e);

	// page up and down are almost the only key events used in ChatWidget
	if (e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown) return;

	if (!(e->modifiers() & Qt::ControlModifier) && !(e->modifiers() & Qt::AltModifier))
	{
		emit keyPressed(e);
	}
	else if (e->modifiers() == Qt::ControlModifier && (e->key() == Qt::Key_V || e->key() == Qt::Key_Insert))
	{
		emit keyPressed(e);
	}
}

void ChatWidget::animate(int frame)
{
	// only enable animations if chosen
	if (frame > 0 && (!m_focus || !ConfigFile::getInstance()->getEnableAnimations())) return;

	if (QMovie* movie = qobject_cast<QMovie*>(sender()))
	{
		// update images in document
		document()->addResource(QTextDocument::ImageResource, m_urls.value(movie), movie->currentPixmap());

		QDateTime current = QDateTime::currentDateTime();

		// to improve performances, only reload page at regular interval
		if (m_lastReload.msecsTo(current) > ConfigFile::getInstance()->getAnimationFrameDelay())
		{
			m_lastReload = current;

			// to be sure, it won't happen when appending HTML
			QMutexLocker lock(&m_contentMutex);

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

AnimationStart::AnimationStart(const QString &url, const QString &file, ChatWidget *widget):QObject(widget), m_widget(widget), m_url(url), m_file(file)
{
	int delayAnimation = ConfigFile::getInstance()->getAnimationFrameDelay();

	QTimer::singleShot(delayAnimation, this, SLOT(timeout()));
}

void AnimationStart::timeout()
{
	m_widget->addAnimation(m_url, m_file);
}
