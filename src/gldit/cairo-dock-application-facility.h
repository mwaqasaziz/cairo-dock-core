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

#ifndef __CAIRO_DOCK_APPLICATION_FACILITY__
#define  __CAIRO_DOCK_APPLICATION_FACILITY__

#include <glib.h>
#include <X11/Xlib.h>

#include "cairo-dock-struct.h"
G_BEGIN_DECLS

/*
*@file cairo-dock-application-facility.h A set of utilities for handling appli-icons.
*/

void cairo_dock_appli_demands_attention (Icon *icon);

void cairo_dock_appli_stops_demanding_attention (Icon *icon);

void cairo_dock_animate_icon_on_active (Icon *icon, CairoDock *pParentDock);

/** Launch a command and play the opening animation during max 10 seconds 
*@param pIcon the icon which launch the command and will be animated
*@param cCommand the command
*@param cWorkingDirectory the working directory (can be NULL)
*@return TRUE if the command has been launched correctly
*/
gboolean cairo_dock_launch_command_with_opening_animation_full (Icon *pIcon, const gchar *cCommand, gchar *cWorkingDirectory);
#define cairo_dock_launch_command_with_opening_animation(pIcon) cairo_dock_launch_command_with_opening_animation_full (pIcon, pIcon->cCommand, pIcon->cWorkingDirectory);


CairoDock *cairo_dock_insert_appli_in_dock (Icon *icon, CairoDock *pMainDock, gboolean bAnimate);

CairoDock *cairo_dock_detach_appli (Icon *pIcon);


void cairo_dock_set_one_icon_geometry_for_window_manager (Icon *icon, CairoDock *pDock);

void cairo_dock_reserve_one_icon_geometry_for_window_manager (GldiWindowActor *pAppli, Icon *icon, CairoDock *pMainDock);


const CairoDockImageBuffer *cairo_dock_appli_get_image_buffer (Icon *pIcon);

G_END_DECLS
#endif
