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
#include "utils.h"

#ifdef Q_OS_MAC

void CreateWindowsList(QAbstractItemModel *model)
{
}

bool RestoreMinimizedWindow(WId &id)
{
	return false;
}

void MinimizeWindow(WId id)
{
}

bool IsUsingComposition()
{
	return true;
}

void PutForegroundWindow(WId id)
{
}

bool IsOS64bits()
{
	return true;
}

bool InitSystemProgress()
{
	return false;
}

bool UninitSystemProgress()
{
	return false;
}

bool BeginSystemProgress()
{
	return false;
}

bool UpdateSystemProgress(qint64 value, qint64 total)
{
	return false;
}

bool EndSystemProgress()
{
	return false;
}

#endif
