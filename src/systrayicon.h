/*
 *  kdAmn is a deviantART Messaging Network client
 *  Copyright (C) 2013  Cedric OCHS
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

#ifndef SYSTRAYICON_H
#define SYSTRAYICON_H

enum SystrayStatus
{
	StatusUndefined,
	StatusNormal,
	StatusTalkOther,
	StatusTalkMe
};

typedef QMap<QString, SystrayStatus> SystrayStatuses;
typedef SystrayStatuses::iterator SystrayStatusesIterator;
typedef SystrayStatuses::const_iterator SystrayStatusesConstIterator;

class SystrayIcon : public QObject
{
	Q_OBJECT

public:
	SystrayIcon(QObject* parent);
	virtual ~SystrayIcon();

	static SystrayIcon* getInstance() { return s_instance; }

	SystrayStatus getStatus(const QString &channel) const;
	void setStatus(const QString &channel, SystrayStatus status);

private:
	bool create();
	void updateStatus();

	static SystrayIcon* s_instance;

	SystrayStatuses m_channels;
	SystrayStatus m_status;
	QSystemTrayIcon *m_icon;
};

#endif
