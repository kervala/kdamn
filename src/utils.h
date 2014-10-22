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

#ifndef UTILS_H
#define UTILS_H

class QAbstractItemModel;

void CreateWindowsList(QAbstractItemModel *model);
bool RestoreMinimizedWindow(WId &id);
void MinimizeWindow(WId id);
void PutForegroundWindow(WId id);
bool IsUsingComposition();
bool IsOS64bits();

void SetMainWindowId(WId id);
WId GetMainWindowId();

bool InitSystemProgress();
bool UninitSystemProgress();
bool BeginSystemProgress();
bool UpdateSystemProgress(qint64 value, qint64 total);
bool EndSystemProgress();

QString encodeEntities(const QString& src, const QString& force = QString());
QString decodeEntities(const QString& src);

#endif
