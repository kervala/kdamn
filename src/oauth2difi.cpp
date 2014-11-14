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
#include "oauth2.h"
#include "utils.h"
#include "cookies.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

/*
	DiFi :
	http://botdom.com/documentation/DiFi
	https://github.com/danopia/deviantart-difi/wiki

	Ccommands :

	* Notes :

		- display_folder
		- display_notes (display note content)
		- display (complately useless)

*/


bool OAuth2::requestMessageFolders()
{
	if (!m_logged)
	{
		m_actions.push_front(ActionCheckFolders);

		return login();
	}

	return get(QString("%1?c[]=MessageCenter;get_folders;&t=json").arg(DIFI_URL));
}

bool OAuth2::requestMessageViews()
{
	if (!m_inboxId)
	{
		m_actions.push_front(ActionCheckNotes);

		return requestMessageFolders();
	}

	return get(QString("%1?c[]=MessageCenter;get_views;%2,oq:notes_unread:0:0:f&t=json").arg(DIFI_URL).arg(m_inboxId));
}

bool OAuth2::requestDisplayFolder(const QString &folderId, int offset)
{
#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	// number/string, i.e. 1 for inbox, 2 for sent, as well as 'starred', 'drafts', 'unread', and custom folders.

	// \"unread\"

	params.addQueryItem("c[]", QString("\"Notes\",\"display_folder\",[%1,%2,false]").arg(folderId).arg(offset));
	params.addQueryItem("t", "json");
	params.addQueryItem("ui", qobject_cast<Cookies*>(m_manager->cookieJar())->get("userinfo"));

	QByteArray data;

#ifdef USE_QT5
	data = params.query().toUtf8();
#else
	data = params.encodedQuery();
#endif

	return post(DIFI_URL, data, HTTPS_URL);
}

bool OAuth2::requestDisplayNote(const QString &folderId, int noteId)
{
	if (!m_manager) return false;

#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("c[]", QString("\"Notes\",\"display_note\",[%1,%2]").arg(folderId).arg(noteId));
	params.addQueryItem("t", "json");
	params.addQueryItem("ui", qobject_cast<Cookies*>(m_manager->cookieJar())->get("userinfo"));

	QByteArray data;

#ifdef USE_QT5
	data = params.query().toUtf8();
#else
	data = params.encodedQuery();
#endif

	return post(DIFI_URL, data, HTTPS_URL);
}

void OAuth2::processDiFi(const QByteArray &content)
{
	// we received JSON response
	QString status, error, errorDescription;

	if (!content.isEmpty())
	{
#ifdef USE_QT5
		QJsonParseError jsonError;
		QJsonDocument doc = QJsonDocument::fromJson(content, &jsonError);

		if (jsonError.error != QJsonParseError::NoError)
		{
			emit errorReceived(jsonError.errorString());
			return;
		}

		QVariantMap map = doc.toVariant().toMap();

		QVariantMap difi = map["DiFi"].toMap();

		if (!difi.isEmpty())
		{
			QVariantMap response = difi["response"].toMap();

			QVariantList calls = response["calls"].toList();

			foreach (const QVariant &itemValue, calls)
			{
				QVariantMap item = itemValue.toMap();

				QVariantMap request = item["request"].toMap();
				QString method = request["method"].toString();

				QVariantMap response = item["response"].toMap();

				status = response["status"].toString();

				if (status == "SUCCESS")
				{
					if (method == "get_folders")
					{
						QVariantList content = response["content"].toList();

						foreach(const QVariant &folderValue, content)
						{
							QVariantMap folder = folderValue.toMap();

							QString folderId = folder["folderid"].toString();
							QString title = folder["title"].toString();
							bool isInbox = folder["is_inbox"].toBool();

							if (isInbox)
							{
								m_inboxId = folderId.toInt();
							}
						}
					}
					else if (method == "get_views")
					{
						QVariantList views = response["content"].toList();

						foreach(const QVariant &viewValue, views)
						{
							QVariantMap view = viewValue.toMap();

							QString offset = view["offset"].toString();
							QString length = view["length"].toString();
							int status = view["status"].toInt();

							QVariantMap result = view["result"].toMap();

							QString matches = result["matches"].toString();
//							int count = result["count"].toInt();

//							QJsonArray hits = result["hits"].toArray();

							emit notesReceived(matches.toInt());
						}
					}
					else if (method == "display_folder")
					{
						QVariantMap data = response["content"].toMap();

						QString folderId = data["folderid"].toString();
						int offset = data["offset"].toInt();

						QString html = QString::fromUtf8(data["body"].toByteArray());

						if (!m_folders.contains(folderId))
						{
							// search notes count per page and max offset
							QRegExp reg("offset=([0-9]+)");

							int pos = 0;
							int min = -1;
							int max = -1;

							while((pos = reg.indexIn(html, pos)) > -1)
							{
								int v = reg.cap(1).toInt();

								if ((v < min) || (min == -1)) min = v;
								if ((v > max) || (max == -1)) max = v;

								pos += reg.matchedLength();
							}

							// init a new folder
							Folder folder;
							folder.id = folderId;
							folder.count = min;
							folder.maxOffset = max;
							folder.notes.reserve(max + min); // optimize memory allocation

							m_folders[folderId] = folder;
						}

						// update current offset
						m_folders[folderId].offset = offset;

						// hack to fix invalid HTML code
						int pos1 = html.indexOf("<div class=\"footer\">");

						if (pos1 > -1)
						{
							int pos2 = html.indexOf("        </ul>", pos1);

							if (pos2 > -1)
							{
								html = html.remove(pos1, pos2-pos1+1);
							}
						}

						// parse HTML code to retrieve notes details
						parseFolder(html, m_folders[folderId]);

						if (offset >= m_folders[folderId].maxOffset)
						{
							emit folderReceived(folderId);
						}
						else
						{
							int nextOffset = m_folders[folderId].offset + m_folders[folderId].count;

							requestDisplayFolder(folderId, nextOffset);
						}
					}
					else if (method == "display_note")
					{
						QVariantMap data = response["content"].toMap();

						QString noteId = QString::number(data["noteid"].toInt());
						QString html = QString::fromUtf8(data["body"].toByteArray());

						QMap<QString, Folder>::iterator it = m_folders.begin(), iend = m_folders.end();

						Note note;
						note.id = noteId;

						while(it != iend)
						{
							int index = it->notes.indexOf(note);

							if (index > -1)
							{
								parseNote(html, it->notes[index]);
							}

							++it;
						}
					}
					else if (method == "display")
					{
						// complately useless, content is null
					}
				}
				else
				{
					QVariantMap content = response["content"].toMap();
					QVariantMap err = content["error"].toMap();

					QString errorCode = err["code"].toString();
					QString errorHuman = err["human"].toString();

					status = "error";
					error = errorCode;
					errorDescription = tr("%1 (%2)").arg(errorDescription).arg(errorCode);
				}
			}
		}
		else
		{
			qWarning() << "JSON data not processed (not valid DiFi)" << content;
		}
#else
		QScriptEngine engine;
		QScriptValue object = engine.evaluate("(" + QString(content) + ")");
#endif
	}

	// TODO: equivalent for Qt 4

	if (status == "error")
	{
		emit errorReceived(tr("DiFi error: %1").arg(errorDescription.isEmpty() ? error:errorDescription));
	}

	processNextAction();
}

bool OAuth2::parseFolder(const QString &html, Folder &folder)
{
	QDomDocument doc;

	QString error;
	int line, column;

	if (!doc.setContent("<root>" + html + "</root>", &error, &line, &column))
	{
		qDebug() << error << line << column;

		return false;
	}

	QDomNode root = doc.firstChild().firstChild().firstChild().firstChild(); // root div ul li

	while (!root.isNull())
	{
		Note note;

		QDomNode node = root.firstChild().firstChildElement("div"); // div useless div

		// subject
		QDomNode subjectNode = node.firstChild();
		QDomNode infoNode = subjectNode.firstChild();
		note.id = infoNode.toElement().attribute("data-noteid");
		note.folderId = infoNode.toElement().attribute("data-folderid");
		note.subject = infoNode.firstChild().toText().data();

		// sender
		QDomNode senderNode = subjectNode.nextSibling();
		note.sender = senderNode.firstChild().nextSibling().firstChild().firstChild().toText().data();

		// date
		QDomNode dateNode = node.nextSibling();

		QString dateAttribute = dateNode.toElement().attribute("title");
		QString dateText = dateNode.firstChild().toText().data();

		note.date = convertDateToISO(dateText.indexOf("ago") > -1 ? dateAttribute:dateText);

		// preview
		QDomNode previewNode = dateNode.nextSiblingElement("div");
		note.preview = previewNode.firstChild().toText().data().trimmed();

		if (!note.id.isEmpty() && !folder.notes.contains(note)) folder.notes << note;

		root = root.nextSibling();
	}

	// TODO: sort

	return true;
}

bool OAuth2::parseNote(const QString &html, Note &note)
{
	QDomDocument doc;

	QString error;
	int line, column;

	if (!doc.setContent("<root>" + html + "</root>", &error, &line, &column))
	{
		qDebug() << error << line << column;

		return false;
	}

	QDomNode root = doc.firstChild().firstChild().firstChild().firstChild(); // root div div form

	QDomNode ch = root.firstChildElement("div");

	if (ch.isNull()) return false;

	QDomNode textarea = ch.firstChild().firstChild().firstChildElement("textarea");
	note.text = textarea.firstChild().toText().data();

	// extract note HTML in another XML document
	QDomDocument out;
	out.appendChild(out.importNode(textarea.nextSibling(), true));
	note.html = out.toString();

	return true;
}
