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

#ifndef SERVERFRAME_H
#define SERVERFRAME_H

#include "ui_serverframe.h"

#include "tabframe.h"

struct DAmnMember;

class ServerFrame : public TabFrame, public Ui::ServerFrame
{
	Q_OBJECT

public:
	ServerFrame(QWidget *parent);
	virtual ~ServerFrame();

	void setSystem(const QString &text);
	void setError(const QString &error);
};

#endif
