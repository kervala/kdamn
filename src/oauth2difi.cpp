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

// Working
bool OAuth2::requestMessageCenterGetViews()
{
	if (!m_inboxId) return false;

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

// Working
bool OAuth2::requestNotesDisplayFolder(const QString &folderId, int offset)
{
	// number/string, i.e. 1 for inbox, 2 for sent, as well as 'starred', 'drafts', 'unread', and custom folders.
	return requestNotes("display_folder", QString("%1,%2,false").arg(folderId).arg(offset));
}

// Working
bool OAuth2::requestNotesDisplayNote(const QString &folderId, const QString &noteId)
{
	return requestNotes("display_note", QString("%1,%2").arg(folderId).arg(noteId));
}

// Working
bool OAuth2::requestNotesMarkAsRead(const QStringList &notesIds)
{
	return requestNotes("mark_as_read", QString("[%1]").arg(notesIds.join(",")));
}

// Working
bool OAuth2::requestNotesMarkAsUnread(const QStringList &notesIds)
{
	return requestNotes("mark_as_unread", QString("[%1]").arg(notesIds.join(",")));
}

// Working
bool OAuth2::requestNotesMove(const QStringList &notesIds, const QString &folderId)
{
	return requestNotes("move", QString("[%1],%2").arg(notesIds.join(",")).arg(folderId));
}

// Working
bool OAuth2::requestNotesPlaceboCall()
{
	return requestNotes("placebo_call", "");
}

// Working
bool OAuth2::requestNotesPreview(const QString &text, bool includeSignature)
{
	return requestNotes("preview", QString("\"%1\",%2").arg(text).arg(includeSignature ? "true":"false"));
}

// Working
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
		QVariantMap map;

#ifdef USE_QT5
		QJsonParseError jsonError;
		QJsonDocument doc = QJsonDocument::fromJson(content, &jsonError);

		if (jsonError.error != QJsonParseError::NoError)
		{
			emit errorReceived(jsonError.errorString());
			return;
		}

		map = doc.toVariant().toMap();
#else
		QScriptEngine engine;
		map = engine.evaluate("(" + QString(content) + ")").toVariant().toMap();
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
								// completely useless, content is null
							}
							else if (method == "display_folder")
							{
								parseNotesDisplayFolder(response);
							}
							else if (method == "display_note")
							{
								parseNotesDisplayNote(response);
							}
							else if (method == "mark_as_read")
							{
								parseNotesMarkAsRead(response);
							}
							else if (method == "mark_as_unread")
							{
								parseNotesMarkAsUnread(response);
							}
							else if (method == "move")
							{
								parseNotesMove(response);
							}
							else if (method == "placebo_call")
							{
								// completely useless, content is null
							}
							else if (method == "preview")
							{
								parseNotesPreview(response);
							}
							else if (method == "rename_folder")
							{
								parseNotesRenameFolder(response);
							}
							else if (method == "save_draft")
							{
								parseNotesSaveDraft(response);
							}
							else if (method == "star")
							{
								parseNotesStar(response);
							}
							else if (method == "unstar")
							{
								parseNotesUnstar(response);
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

	if (status == "error")
	{
		emit errorReceived(tr("DiFi error: %1").arg(errorDescription.isEmpty() ? error:errorDescription));
	}

	processNextAction();
}

bool OAuth2::parseFolder(const QString &html, Folder &folder)
{
	QRegExp rootReg("data-noteid=\"([0-9]+)\" class=\"([a-z ]+)\"");

	int pos = 0, lastPos = 0;

	while((pos = rootReg.indexIn(html, pos)) > -1)
	{
		lastPos = pos;

		Note note;
		note.id = rootReg.cap(1);

		QString classes = rootReg.cap(2);

		note.unread = classes.indexOf("unread") > -1;
		note.replied = classes.indexOf("replied") > -1;

		QRegExp reg;

		reg.setPattern("class=\"icon_star ( starred)?\"");

		pos = reg.indexIn(html, pos);

		if (pos > -1)
		{
			note.starred = !reg.cap(1).isEmpty();

			lastPos = pos;
		}
		else
		{
			pos = lastPos;
		}

		// subject
		reg.setPattern("data-noteid=\"([0-9]+)\" data-folderid=\"([0-9]+)\" href=\"#([0-9_/]+)\" title=\"([^\"]+)\" class=\"wrap-for-ts-(abs|rel)\">([^<]+)");

		pos = reg.indexIn(html, pos);

		if (pos > -1)
		{
			note.id = reg.cap(1);
			note.folderId = reg.cap(2);
			note.subject = reg.cap(6);

			lastPos = pos;
		}
		else
		{
			pos = lastPos;
		}

		// sender
		reg.setPattern("username\"([^>]*)>([^<]+)");

		pos = reg.indexIn(html, pos);

		if (pos > -1)
		{
			note.sender = reg.cap(2);

			lastPos = pos;
		}
		else
		{
			pos = lastPos;
		}

		// date
		reg.setPattern("<span class=\"ts\" title=\"([^\"]+)\">([^<]+)");

		pos = reg.indexIn(html, pos);

		if (pos > -1)
		{
			QString dateAttribute = reg.cap(1);
			QString dateText = reg.cap(2);

			note.date = convertDateToISO(dateText.indexOf("ago") > -1 ? dateAttribute:dateText);

			lastPos = pos;
		}
		else
		{
			pos = lastPos;
		}

		// preview
		reg.setPattern("<div class=\"note-preview expandable\">([^<]+)");

		pos = reg.indexIn(html, pos);

		if (pos > -1)
		{
			note.preview = reg.cap(1).trimmed();
		}
		else
		{
			pos = lastPos;
		}

		folder.addNote(note);
	}

	// TODO: sort

	return true;
}

bool OAuth2::parseNote(const QString &html, Note &note)
{
	QRegExp reg;

	reg.setPattern("<span class=\"mcb-ts\" title=\"([^\"]+)\">([^<]+)</span>");

	int pos = reg.indexIn(html);

	if (pos > -1)
	{
		QString dateAttribute = reg.cap(1);
		QString dateText = reg.cap(2);

		note.date = convertDateToISO(dateText.indexOf("ago") > -1 ? dateAttribute:dateText);

		pos += reg.matchedLength();
	}

	reg.setPattern("<span class=\"mcb-title\">([^<]+)</span>");

	pos = reg.indexIn(html, pos);

	if (pos > -1)
	{
		note.subject = reg.cap(1);

		pos += reg.matchedLength();
	}

	reg.setPattern("<span class=\"mcb-from\" username=\"([^\"]+)\">");

	pos = reg.indexIn(html, pos);

	if (pos > -1)
	{
		note.sender = reg.cap(1);

		pos += reg.matchedLength();
	}

	reg.setPattern("<textarea class=\"mcb-body\" style=\"display: none\">(.+)</textarea>");

	pos = reg.indexIn(html, pos);

	if (pos > -1)
	{
		note.text = decodeEntities(reg.cap(1));

		pos += reg.matchedLength();
	}

	reg.setPattern("<div class=\"mcb-body wrap-text\">(.+)</div>");
	reg.setMinimal(true);

	pos = reg.indexIn(html, pos);

	if (pos > -1)
	{
		note.html = reg.cap(1);

		pos += reg.matchedLength();
	}

	return true;
}

bool OAuth2::parseNotesIdsAndCount(const QVariantMap &response, QStringList &notesIds, int &count)
{
	QVariantMap data = response["content"].toMap();

	QVariantList ids = data["noteids"].toList();

	foreach(const QVariant &id, ids)
	{
		notesIds << QString::number(id.toInt());
	}

	count = data["count"].toString().toInt();

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

	emit foldersReceived();

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

	if (/* !m_folders.contains(folderId) || */ m_folders[folderId].count == 0)
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

		// update folder
		m_folders[folderId].updateValues(min, max);
	}

	// parse HTML code to retrieve notes details
	if (!parseFolder(html, m_folders[folderId])) return false;

	emit notesUpdated(folderId, offset, m_folders[folderId].count);

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

	int index = -1;

	while(it != iend)
	{
		index = it->notes.indexOf(note);

		if (index > -1) break;

		++it;
	}

	if (!parseNote(html, index > -1 ? it->notes[index]:note)) return false;

	if (index == -1)
	{
		// note id not found in folders, or folder not parsed yet
		// TODO: save note somewhere
	}

//	emit notesNoteParsed();

	return true;
}

bool OAuth2::parseNotesMarkAsRead(const QVariantMap &response)
{
	QStringList noteIds;
	int count;

	if (!parseNotesIdsAndCount(response, noteIds, count)) return false;

	emit notesRead(noteIds);

	return true;
}

bool OAuth2::parseNotesMarkAsUnread(const QVariantMap &response)
{
	QStringList noteIds;
	int count;

	if (!parseNotesIdsAndCount(response, noteIds, count)) return false;

	emit notesUnread(noteIds);

	return true;
}

bool OAuth2::parseNotesMove(const QVariantMap &response)
{
	QVariantMap data = response["content"].toMap();

	QList<QString> folders;

	QVariantMap::const_iterator it = data.begin(), iend = data.end();

	while(it != iend)
	{
		QString folderId = it.key();
		int count = it.value().toString().toInt();

		++it;
	}

	emit notesMoved();

	return true;
}

bool OAuth2::parseNotesPreview(const QVariantMap &response)
{
	QString html = response["content"].toString();

	emit notesPreviewReceived(html);

	return true;
}

bool OAuth2::parseNotesRenameFolder(const QVariantMap &response)
{
	QVariantMap data = response["content"].toMap();

	QString name = data["foldername"].toString();
	QString id = QString::number(data["folderid"].toInt());

	// rename folder
	m_folders[id].name = name;

	emit notesFolderRenamed(id, name);

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
	int count;

	if (!parseNotesIdsAndCount(response, noteIds, count)) return false;

	emit notesStarred(noteIds);

	return true;
}

bool OAuth2::parseNotesUnstar(const QVariantMap &response)
{
	QStringList noteIds;
	int count;

	if (!parseNotesIdsAndCount(response, noteIds, count)) return false;

	emit notesUnstarred(noteIds);

	return true;
}
