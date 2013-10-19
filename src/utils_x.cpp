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

#include "common.h"
#include "utils.h"

#ifdef Q_OS_UNIX

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xmu/WinUtil.h>

#ifdef index
	#undef index
#endif

static void print_client_properties(Display *dpy, Window w, QAbstractItemModel *model)
{
	// retrieve window name
	char *name = NULL;
//	Status status = XFetchName(dpy, w, &name);

	if (name && *name != '\0' && model->insertRow(0))
	{
		QPixmap pixmap;

		QModelIndex index = model->index(0, 0);

		model->setData(index, QString(name));
		model->setData(index, pixmap, Qt::DecorationRole);
		model->setData(index, qVariantFromValue((WId)w), Qt::UserRole);
	}

	XFree(name);
}

static void lookat(Display *dpy, Window root, QAbstractItemModel *model)
{
	Window dummy, *children = NULL, client;
	unsigned int nchildren = 0;

	// clients are not allowed to stomp on the root and ICCCM doesn't yet
	// say anything about window managers putting stuff there; but, try
	// anyway.
	print_client_properties (dpy, root, model);

	// then, get the list of windows
	if (!XQueryTree (dpy, root, &dummy, &dummy, &children, &nchildren)) return;

	for (unsigned int i = 0; i < nchildren; ++i)
	{
		client = XmuClientWindow (dpy, children[i]);
		if (client != None)
			print_client_properties (dpy, client, model);
	}

	QFileIconProvider icon;

	if (model->insertRow(0))
	{
		QModelIndex index = model->index(0, 0);

		model->setData(index, QObject::tr("Whole screen"));
		model->setData(index, icon.icon(QFileIconProvider::Desktop).pixmap(32, 32), Qt::DecorationRole);
		model->setData(index, qVariantFromValue((WId)root), Qt::UserRole);
	}
}

void CreateWindowsList(QAbstractItemModel *model)
{
	char *displayname = NULL;
	bool all_screens = false;

	Display *dpy = XOpenDisplay (displayname);

	if (!dpy) return;

	if (all_screens)
	{
		for (int i = 0; i < ScreenCount(dpy); ++i)
			lookat (dpy, RootWindow(dpy,i), model);
	}
	else
	{
		lookat (dpy, DefaultRootWindow(dpy), model);
	}

	XCloseDisplay (dpy);

	model->sort(0);
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
