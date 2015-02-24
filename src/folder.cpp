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
#include "folder.h"

#ifdef DEBUG_NEW
	#define new DEBUG_NEW
#endif

Folder::Folder():count(0), maxOffset(0)
{
}

bool Folder::updateValues(int count, int maxOffset)
{
	this->count = count;
	this->maxOffset = maxOffset;

	// optimize memory allocation
	this->notes.reserve(count + maxOffset);

	return true;
}

bool Folder::load()
{
	QFile file("test.json");

	if (!file.open(QFile::ReadOnly)) return false;

	QByteArray data = file.readAll();

	QVariantMap map;

#ifdef USE_QT5
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(data, &error);

	if (doc.isEmpty()) return false;

	map = doc.toVariant().toMap();
#else
	QScriptEngine engine;
	map = engine.evaluate("(" + QString(data) + ")").toVariant().toMap();
#endif

	id = map["id"].toString();
	name = map["name"].toString();
	maxOffset = map["max_offset"].toInt();

	notes.reserve(map["count"].toInt());

	notes.clear();

	QVariantList listNotes = map["notes"].toList();

	foreach(const QVariant &v, listNotes)
	{
		QVariantMap mapNote = v.toMap();

		// create a note
		Note note;
		note.id = mapNote["id"].toString();
		note.folderId = mapNote["folder_id"].toString();
		note.subject = mapNote["subject"].toString();
		note.preview = mapNote["preview"].toString();
		note.date = mapNote["date"].toString();
		note.sender = mapNote["from"].toString();
		note.recipients = mapNote["to"].toString().split(", ");
		note.text = mapNote["body_text"].toString();
		note.html = mapNote["body_html"].toString();
		note.starred = mapNote["star"].toBool();
		note.unread = mapNote["unread"].toBool();

		// append note to notes list
		notes << note;
	}

	return true;
}

bool Folder::save()
{
	QVariantMap map;

	map["id"] = id;
	map["name"] = name;
	map["count"] = notes.size();
	map["max_offset"] = maxOffset;

	QVariantList listNotes;

	foreach(const Note &note, notes)
	{
		// create a note
		QVariantMap mapNote;
		mapNote["id"] = note.id;
		mapNote["folder_id"] = note.folderId;
		mapNote["subject"] = note.subject;
		mapNote["preview"] = note.preview;
		mapNote["date"] = note.date;
		mapNote["from"] = note.sender;
		mapNote["to"] = note.recipients.join(", ");
		mapNote["body_text"] = note.text;
		mapNote["body_html"] = note.html;
		mapNote["star"] = note.starred;
		mapNote["unread"] = note.unread;

		// append note to notes list
		listNotes << mapNote;
	}

	map["notes"] = listNotes;

	QByteArray data;

#ifdef USE_QT5
	QJsonObject obj = QJsonObject::fromVariantMap(map);

	QJsonDocument doc;
	doc.setObject(obj);

	data = doc.toJson();
#else
	// TODO: find equivalent
#endif

	QFile file("test.json");

	if (!file.open(QFile::WriteOnly)) return false;

	file.write(data);

	return true;
}

bool Folder::addNote(const Note &note)
{
	if (note.id.isEmpty()) return false;

	int pos = notes.indexOf(note);

	if (pos > -1) return false;

	notes.append(note);

	return true;
}

bool Folder::exportNotes(const QString &dir)
{
	return true;
}
