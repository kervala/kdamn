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

	* MessageCenter

		- get_folders
		- get_views

	* Notes :

		- display_folder
		- display_note (display note content)
		- display (complately useless)

*/

// helper to query GET DiFi methods
bool OAuth2::requestGet(const QString &cls, const QString &method, const QString &args)
{
	if (!m_manager || !m_logged) return false;

	return get(QString("%1?c[]=%2;%3;%4&t=json").arg(DIFI_URL).arg(cls).arg(method).arg(args), HTTPS_URL);
}

// helper to query POST DiFi methods
bool OAuth2::requestPost(const QString &cls, const QString &method, const QString &args)
{
	if (!m_manager || !m_logged) return false;

#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("c[]", QString("\"%1\",\"%2\",[%3]").arg(cls).arg(method).arg(args));
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

// MessageCenter

// helper to query MessageCenter DiFi methods
bool OAuth2::requestMessageCenter(const QString &method, const QString &args)
{
	return requestGet("MessageCenter", method, args);
}

// Working
bool OAuth2::requestMessageCenterGetFolders()
{
	return requestMessageCenter("get_folders", "");
}

bool OAuth2::requestMessageCenterGetViews()
{
	if (!m_inboxId)
	{
		m_actions.push_front(ActionCheckNotes);

		return requestMessageCenterGetFolders();
	}

	return requestMessageCenter("get_views", QString("%1,oq:notes_unread:0:0:f").arg(m_inboxId));
}

// helper to query Notes DiFi methods
bool OAuth2::requestNotes(const QString &method, const QString &args)
{
	return requestPost("Notes", method, args);
}

// Working
bool OAuth2::requestNotesCreateFolder(const QString &name, const QString &parentId)
{
	return requestNotes("create_folder", QString("\"%1\",%2").arg(name).arg(parentId));
}

// Working
bool OAuth2::requestNotesDelete(const QStringList &notesIds)
{
	return requestNotes("delete", QString("[%1]").arg(notesIds.join(",")));
}

// Working
bool OAuth2::requestNotesDeleteFolder(const QString &folderId)
{
	return requestNotes("delete_folder", folderId);
}

bool OAuth2::requestNotesDisplayFolder(const QString &folderId, int offset)
{
	// number/string, i.e. 1 for inbox, 2 for sent, as well as 'starred', 'drafts', 'unread', and custom folders.
	// \"unread\"
	return requestNotes("display_folder", QString("%1,%2,false").arg(folderId).arg(offset));
}

bool OAuth2::requestNotesDisplayNote(const QString &folderId, const QString &noteId)
{
	return requestNotes("display_note", QString("%1,%2").arg(folderId).arg(noteId));
}

bool OAuth2::requestNotesMarkAsRead(const QStringList &notesIds)
{
	return requestNotes("mark_as_read", QString("%1").arg(notesIds.join(",")));
}

bool OAuth2::requestNotesMarkAsUnread(const QStringList &notesIds)
{
	return requestNotes("mark_as_unread", QString("%1").arg(notesIds.join(",")));
}

bool OAuth2::requestNotesMove(const QStringList &notesIds, const QString &folderId)
{
	return requestNotes("move", QString("%1,%2").arg(notesIds.join(",")).arg(folderId));
}

// Working
bool OAuth2::requestNotesPlaceboCall()
{
	return requestNotes("placebo_call", "");
}

bool OAuth2::requestNotesPreview(const QString &text, bool includeSignature)
{
	return requestNotes("preview", QString("\"%1\",%2").arg(text).arg(includeSignature ? "true":"false"));
}

bool OAuth2::requestNotesRenameFolder(const QString &folderId, const QString &folderName)
{
	return requestNotes("rename_folder", QString("%1,\"%2\"").arg(folderId).arg(folderName));
}

// Working
bool OAuth2::requestNotesSaveDraft(const QString &recipients, const QString &subject, const QString &body)
{
	// TODO: check with other values instead of 0
	// TODO: escape special characters in subject and body
	return requestNotes("save_draft", QString("\"%1\",\"%2\",\"%3\",0").arg(recipients).arg(subject).arg(body));
}

// Working
bool OAuth2::requestNotesStar(const QStringList &notesIds)
{
	return requestNotes("star", QString("[%1]").arg(notesIds.join(",")));
}

// Working
bool OAuth2::requestNotesUnstar(const QStringList &notesIds)
{
	return requestNotes("unstar", QString("[%1]").arg(notesIds.join(",")));
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
#else
		QScriptEngine engine;
		QScriptValue object = engine.evaluate("(" + QString(content) + ")");
#endif

		QVariantMap difi = map["DiFi"].toMap();

		if (!difi.isEmpty())
		{
			status = difi["status"].toString();
			QVariantMap response = difi["response"].toMap();

			if (status == "FAIL")
			{
				status = "error";
				error = response["error"].toString();
				errorDescription = QString("FAIL: %1 (%2)").arg(error).arg(response["details"].toString());
			}
			else if (status == "SUCCESS")
			{
				QVariantList calls = response["calls"].toList();

				foreach (const QVariant &itemValue, calls)
				{
					QVariantMap item = itemValue.toMap();

					QVariantMap request = item["request"].toMap();
					QString cls = request["class"].toString();
					QString method = request["method"].toString();

					QVariantMap response = item["response"].toMap();

					status = response["status"].toString();

					if (status == "SUCCESS")
					{
						if (cls == "MessageCenter")
						{
							if (method == "get_folders")
							{
								parseMessageCenterGetFolders(response);
							}
							else if (method == "get_views")
							{
								parseMessageCenterGetViews(response);
							}
							else
							{
								qDebug() << "MessageCenter:" << method << "not implemented";
							}
						}
						else if (cls == "Notes")
						{
							if (method == "create_folder")
							{
								parseNotesCreateFolder(response);
							}
							else if (method == "delete")
							{
								parseNotesDelete(response);
							}
							else if (method == "delete_folder")
							{
								parseNotesDeleteFolder(response);
							}
							else if (method == "display")
							{
								// complately useless, content is null
							}
							else if (method == "display_folder")
							{
								parseNotesDisplayFolder(response);
							}
							else if (method == "display_note")
							{
								parseNotesDisplayNote(response);
							}
							else if (method == "placebo_call")
							{
								parseNotesPlaceboCall(response);
							}
							else if (method == "star")
							{
								parseNotesStar(response);
							}
							else if (method == "unstar")
							{
								parseNotesUnstar(response);
							}
							else if (method == "save_draft")
							{
								parseNotesSaveDraft(response);
							}
							else
							{
								qDebug() << "Notes:" << method << "not implemented";
							}
						}
						else
						{
							qDebug() << "Class" << cls << "not implemented";
						}
					}
					else if (status == "NOEXEC_HALT")
					{
						QVariantMap data = response["content"].toMap();
						QString error = data["error"].toString();

						QVariantMap errorDetails = data["details"].toMap();

						QString argument = errorDetails["argument"].toString();

						QVariantMap filterError = errorDetails["filter_error"].toMap();

						QString err = filterError["error"].toString();
						QString det = filterError["details"].toString();

						status = "error";
						errorDescription = tr("NOEXEC_HALT: %1 (%2 - %3: %4)").arg(error).arg(argument).arg(err).arg(det);
					}
					else
					{
						QVariantMap data = response["content"].toMap();
						QVariantMap err = data["error"].toMap();

						QString errorCode = err["code"].toString();
						QString errorHuman = err["human"].toString();

						status = "error";
						error = errorCode;
						errorDescription = tr("%1 (%2)").arg(errorDescription).arg(errorCode);
					}
				}
			}
		}
		else
		{
			qWarning() << "JSON data not processed (not valid DiFi)" << content;
		}
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

bool OAuth2::parseMessageCenterGetFolders(const QVariantMap &response)
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

	return true;
}

bool OAuth2::parseMessageCenterGetViews(const QVariantMap &response)
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
//		int count = result["count"].toInt();

//		QJsonArray hits = result["hits"].toArray();

		emit notesReceived(matches.toInt());
	}

	return true;
}

bool OAuth2::parseNotesCreateFolder(const QVariantMap &response)
{
	QVariantMap data = response["content"].toMap();

	QString name = data["foldername"].toString();
	QString id = QString::number(data["folderid"].toInt());

	emit notesFolderCreated(name, id);

	return true;
}

bool OAuth2::parseNotesDelete(const QVariantMap &response)
{
	QVariantMap data = response["content"].toMap();

	// TODO: browse all folders IDs
	int count = data["1"].toString().toInt();

	emit notesDeleted();

	return true;
}

bool OAuth2::parseNotesDeleteFolder(const QVariantMap &response)
{
	QVariantMap data = response["content"].toMap();

	QString id = QString::number(data["folderid"].toInt());
	QVariantMap counts = data["counts"].toMap();

	// TODO: browse all folders IDs
	int count = counts["1"].toString().toInt();

	emit notesFolderDeleted(id);

	return true;
}

bool OAuth2::parseNotesDisplayFolder(const QVariantMap &response)
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
	if (parseFolder(html, m_folders[folderId]))
	{
		emit notesUpdated(folderId, offset, m_folders[folderId].count);
	}

	return true;
}

bool OAuth2::parseNotesDisplayNote(const QVariantMap &response)
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

	return true;
}

bool OAuth2::parseNotesMarkAsRead(const QVariantMap &response)
{
	return true;
}

bool OAuth2::parseNotesMarkAsUnread(const QVariantMap &response)
{
	return true;
}

bool OAuth2::parseNotesMove(const QVariantMap &response)
{
	return true;
}

bool OAuth2::parseNotesPlaceboCall(const QVariantMap &response)
{
	// nothing to parse
	return true;
}

bool OAuth2::parseNotesPreview(const QVariantMap &response)
{
	return true;
}

bool OAuth2::parseNotesRenameFolder(const QVariantMap &response)
{
	return true;
}

bool OAuth2::parseNotesSaveDraft(const QVariantMap &response)
{
	QVariantMap data = response["content"].toMap();

	QString draftId = QString::number(data["draftid"].toInt());
	QString count = data["count"].toString();

	emit notesDraftSaved(draftId);

	return true;
}

bool OAuth2::parseNotesStar(const QVariantMap &response)
{
	QStringList noteIds;

	QVariantMap data = response["content"].toMap();

	QVariantList ids = data["noteids"].toList();

	foreach(const QVariant &id, ids)
	{
		noteIds << QString::number(id.toInt());
	}

	QString count = data["count"].toString();

	emit notesStarred(noteIds);

	return true;
}

bool OAuth2::parseNotesUnstar(const QVariantMap &response)
{
	QStringList noteIds;

	QVariantMap data = response["content"].toMap();

	QVariantList ids = data["noteids"].toList();

	foreach(const QVariant &id, ids)
	{
		noteIds << QString::number(id.toInt());
	}

	QString count = data["count"].toString();

	emit notesUnstarred(noteIds);

	return true;
}
