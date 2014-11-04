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

#ifdef USE_QT5
#define GET_JSON(x) object[x]
#define GET_JSON_STRING(x) object[x].toString()
#define GET_JSON_DOUBLE(x) object[x].toDouble()
#else
#define GET_JSON(x) object.property(x)
#define GET_JSON_STRING(x) object.property(x).toString()
#define GET_JSON_DOUBLE(x) object.property(x).toNumber()
#endif

// DiFi :
// http://botdom.com/documentation/DiFi
// https://github.com/danopia/deviantart-difi/wiki

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

bool OAuth2::requestNotes()
{
#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("c[]", "\"Notes\",\"display_folder\",[\"unread\",0,false]");
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

bool OAuth2::requestDisplayNote(int noteId)
{
#ifdef USE_QT5
	QUrlQuery params;
#else
	QUrl params;
#endif

	params.addQueryItem("c[]", QString("\"Notes\",\"display_note\",[%1,%2]").arg(m_inboxId).arg(noteId));
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

#ifdef USE_QT5
	QJsonParseError jsonError;
	QJsonDocument doc = QJsonDocument::fromJson(content, &jsonError);

	if (jsonError.error != QJsonParseError::NoError)
	{
		emit errorReceived(jsonError.errorString());
		return;
	}

	QJsonObject object = doc.object();

	QJsonObject difi = object["DiFi"].toObject();

	if (!difi.isEmpty())
	{
		QJsonObject response = difi["response"].toObject();

		QJsonArray calls = response["calls"].toArray();

		foreach (const QJsonValue &itemValue, calls)
		{
			QJsonObject item = itemValue.toObject();

			QJsonObject request = item["request"].toObject();
			QString method = request["method"].toString();

			QJsonObject response = item["response"].toObject();

			status = response["status"].toString();

			if (status == "SUCCESS")
			{
				if (method == "get_folders")
				{
					QJsonArray content = response["content"].toArray();

					foreach(const QJsonValue &folderValue, content)
					{
						QJsonObject folder = folderValue.toObject();

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
					QJsonArray views = response["content"].toArray();

					foreach(const QJsonValue &viewValue, views)
					{
						QJsonObject view = viewValue.toObject();

						QString offset = view["offset"].toString();
						QString length = view["length"].toString();
						int status = view["status"].toInt();

						QJsonObject result = view["result"].toObject();

						QString matches = result["matches"].toString();
//						int count = result["count"].toInt();

//						QJsonArray hits = result["hits"].toArray();

						emit notesReceived(matches.toInt());
					}
				}
			}
			else
			{
				QJsonObject content = response["content"].toObject();
				QJsonObject err = content["error"].toObject();

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

	// TODO: equivalent for Qt 4

	if (status == "error")
	{
		emit errorReceived(tr("DiFi error: %1").arg(errorDescription.isEmpty() ? error:errorDescription));
	}
	else
	{
		processNextAction();
	}
}
