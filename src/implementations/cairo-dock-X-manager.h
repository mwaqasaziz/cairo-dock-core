/*
* This file is a part of the Cairo-Dock project
*
* Copyright : (C) see the 'copyright' file.
* E-mail    : see the 'copyright' file.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 3
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __CAIRO_DOCK_X_MANAGER__
#define  __CAIRO_DOCK_X_MANAGER__

#include "cairo-dock-struct.h"
#include "cairo-dock-windows-manager.h"
G_BEGIN_DECLS

/*
*@file cairo-dock-X-manager.h This class manages the interactions with X.
* The X manager handles signals from X and dispatch them to the Windows manager and the Desktop manager.
*/

typedef struct _GldiXManager CairoDockDesktopManager;

// no param (and the manager is not exported)

// manager
typedef struct _GldiXManager GldiXManager;
struct _GldiXManager {
	GldiManager mgr;
	} ;

// signals
typedef enum {
	NB_NOTIFICATIONS_X_MANAGER = NB_NOTIFICATIONS_WINDOWS
	} CairoXManagerNotifications;

// data
typedef struct _GldiXWindowActor GldiXWindowActor;
struct _GldiXWindowActor {
	GldiWindowActor actor;
	// X-specific
	Window Xid;
	gint iLastCheckTime;
	Pixmap iBackingPixmap;
	Window XTransientFor;
	guint iDemandsAttention;  // a mask of XAttentionFlag
	gboolean bIgnored;
	};


void gldi_register_X_manager (void);

G_END_DECLS
#endif