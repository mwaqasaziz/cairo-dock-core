/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 2; tab-width: 2 -*- */
/*
** Login : <ctaf42@gmail.com>
** Started on  Sun Jan 27 18:35:38 2008 Cedric GESTES
** $Id$
**
** Author(s)
**  - Cedric GESTES <ctaf42@gmail.com>
**  - Fabrice REY
**
** Copyright (C) 2008 Cedric GESTES
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/


#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtkgl.h>
#include <GL/gl.h> 
#include <GL/glu.h> 
#include <GL/glx.h> 
#include <gdk/x11/gdkglx.h>

#include <gdk/gdkx.h>
#include "cairo-dock-draw.h"
#include "cairo-dock-modules.h"
#include "cairo-dock-dialogs.h"
#include "cairo-dock-icons.h"
#include "cairo-dock-config.h"
#include "cairo-dock-applications-manager.h"
#include "cairo-dock-notifications.h"
#include "cairo-dock-log.h"
#include "cairo-dock-menu.h"
#include "cairo-dock-dock-factory.h"
#include "cairo-dock-X-utilities.h"
#include "cairo-dock-surface-factory.h"
#include "cairo-dock-renderer-manager.h"
#include "cairo-dock-draw-opengl.h"
#include "cairo-dock-load.h"
#include "cairo-dock-internal-desklets.h"
#include "cairo-dock-internal-system.h"
#include "cairo-dock-internal-background.h"
#include "cairo-dock-desklet.h"

extern CairoDock *g_pMainDock;
extern int g_iXScreenWidth[2], g_iXScreenHeight[2];
extern gchar *g_cConfFile;
extern gboolean g_bSticky;
extern gboolean g_bUseOpenGL;
extern gboolean g_bIndirectRendering;
extern GdkGLConfig* g_pGlConfig;

static cairo_surface_t *s_pRotateButtonSurface = NULL;
static cairo_surface_t *s_pRetachButtonSurface = NULL;
static GLuint iRotateButtonTexture = 0;
static GLuint iRetachButtonTexture = 0;

void cairo_dock_load_desklet_buttons (cairo_t *pSourceContext)
{
	if (s_pRotateButtonSurface != NULL)
	{
		cairo_surface_destroy (s_pRotateButtonSurface);
		s_pRotateButtonSurface = NULL;
	}
	s_pRotateButtonSurface = cairo_dock_load_image_for_icon (pSourceContext,
		myDesklets.cRotateButtonImage,
		myDesklets.iDeskletButtonSize,
		myDesklets.iDeskletButtonSize);
	if (s_pRotateButtonSurface == NULL)
	{
		gchar *cRotateButtonPath = g_strdup_printf ("%s/%s", CAIRO_DOCK_SHARE_DATA_DIR, "rotate-desklet.svg");
		s_pRotateButtonSurface = cairo_dock_create_surface_for_icon (cRotateButtonPath,
			pSourceContext,
			myDesklets.iDeskletButtonSize,
			myDesklets.iDeskletButtonSize);
		g_free (cRotateButtonPath);
	}
	
	if (s_pRetachButtonSurface != NULL)
	{
		cairo_surface_destroy (s_pRetachButtonSurface);
		s_pRetachButtonSurface = NULL;
	}
	s_pRetachButtonSurface = cairo_dock_load_image_for_icon (pSourceContext,
		myDesklets.cRetachButtonImage,
		myDesklets.iDeskletButtonSize,
		myDesklets.iDeskletButtonSize);
	if (s_pRetachButtonSurface == NULL)
	{
		gchar *cRetachButtonPath = g_strdup_printf ("%s/%s", CAIRO_DOCK_SHARE_DATA_DIR, "retach-desklet.svg");
		s_pRetachButtonSurface = cairo_dock_create_surface_for_icon (cRetachButtonPath, pSourceContext, myDesklets.iDeskletButtonSize, myDesklets.iDeskletButtonSize);
		g_free (cRetachButtonPath);
	}
	
	cairo_dock_load_desklet_buttons_texture ();
}
void cairo_dock_load_desklet_buttons_texture (void)
{
	cd_message ("%s (%x;%x)", __func__, s_pRotateButtonSurface, s_pRetachButtonSurface);
	if (! g_bUseOpenGL)
		return ;
	
	if (iRotateButtonTexture != 0)
	{
		glDeleteTextures (1, &iRotateButtonTexture);
		iRotateButtonTexture = 0;
	}
	if (iRetachButtonTexture != 0)
	{
		glDeleteTextures (1, &iRetachButtonTexture);
		iRetachButtonTexture = 0;
	}
	if (s_pRotateButtonSurface != NULL)
	{
		iRotateButtonTexture = cairo_dock_create_texture_from_surface (s_pRotateButtonSurface);
	}
	
	if (s_pRetachButtonSurface != NULL)
	{
		iRetachButtonTexture = cairo_dock_create_texture_from_surface (s_pRetachButtonSurface);
	}
}

static void _cairo_dock_render_desklet (CairoDesklet *pDesklet, GdkRectangle *area)
{
	gint w = 0, h = 0;
	cairo_t *pCairoContext;
	double fColor[4] = {1., 1., 1., 0.};
	if (gtk_window_is_active (GTK_WINDOW (pDesklet->pWidget)))
		fColor[3] = 1.;
	else
		fColor[3] = 1.*pDesklet->iGradationCount / CD_NB_ITER_FOR_GRADUATION;
	
	gboolean bRenderOptimized = (area->x > 0 || area->y > 0);
	/*if (bRenderOptimized)
	{
		//g_print ("Using optimized render\n");
		pCairoContext = cairo_dock_create_drawing_context_on_area (CAIRO_CONTAINER (pDesklet), area, fColor);
	}
	else*/
	{
		//cd_debug ("Using normal render");
		pCairoContext = cairo_dock_create_drawing_context (CAIRO_CONTAINER (pDesklet));
	}
	
	if (pDesklet->fZoom != 1)
	{
		//g_print (" desklet zoom : %.2f (%dx%d)\n", pDesklet->fZoom, pDesklet->iWidth, pDesklet->iHeight);
		cairo_translate (pCairoContext,
			pDesklet->iWidth * (1 - pDesklet->fZoom)/2,
			pDesklet->iHeight * (1 - pDesklet->fZoom)/2);
		cairo_scale (pCairoContext, pDesklet->fZoom, pDesklet->fZoom);
	}
	
	if (fColor[3] != 0)
	{
		cairo_save (pCairoContext);
		cairo_pattern_t *pPattern = cairo_pattern_create_radial (.5*pDesklet->iWidth,
			.5*pDesklet->iHeight,
			0.,
			.5*pDesklet->iWidth,
			.5*pDesklet->iHeight,
			.5*MIN (pDesklet->iWidth, pDesklet->iHeight));
		cairo_pattern_set_extend (pPattern, CAIRO_EXTEND_NONE);
		
		cairo_pattern_add_color_stop_rgba   (pPattern,
			0.,
			fColor[0], fColor[1], fColor[2], fColor[3]);
		cairo_pattern_add_color_stop_rgba   (pPattern,
			1.,
			fColor[0], fColor[1], fColor[2], 0.);
		cairo_set_source (pCairoContext, pPattern);
		cairo_paint (pCairoContext);
		cairo_pattern_destroy (pPattern);
		cairo_restore (pCairoContext);
	}
	
	cairo_save (pCairoContext);
	
	if (pDesklet->fRotation != 0)
	{
		double alpha = atan2 (pDesklet->iHeight, pDesklet->iWidth);
		double theta = fabs (pDesklet->fRotation);
		if (theta > G_PI/2)
			theta -= G_PI/2;
		double fZoomX, fZoomY;
		double d = .5 * sqrt (pDesklet->iWidth * pDesklet->iWidth + pDesklet->iHeight * pDesklet->iHeight);
		fZoomX = fabs (.5 * pDesklet->iWidth / (d * sin (alpha + theta)));
		fZoomY = fabs (.5 * pDesklet->iHeight / (d * cos (alpha - theta)));
		//g_print ("d = %.2f ; alpha = %.2f ; zoom : %.2fx%.2f\n", d, alpha/G_PI*180., fZoomX, fZoomY);
		
		cairo_translate (pCairoContext,
			.5*pDesklet->iWidth,
			.5*pDesklet->iHeight);
		
		cairo_rotate (pCairoContext, pDesklet->fRotation);
		
		cairo_translate (pCairoContext,
			-.5*pDesklet->iWidth * fZoomX,
			-.5*pDesklet->iHeight * fZoomY);
		cairo_scale (pCairoContext, fZoomX, fZoomY);
	}
	
	cairo_save (pCairoContext);
	if (pDesklet->pBackGroundSurface != NULL && pDesklet->fBackGroundAlpha != 0)
	{
		cairo_set_source_surface (pCairoContext,
			pDesklet->pBackGroundSurface,
			0.,
			0.);
		if (pDesklet->fBackGroundAlpha == 1)
			cairo_paint (pCairoContext);
		else
			cairo_paint_with_alpha (pCairoContext, pDesklet->fBackGroundAlpha);
	}
	
	if (pDesklet->iLeftSurfaceOffset != 0 || pDesklet->iTopSurfaceOffset != 0 || pDesklet->iRightSurfaceOffset != 0 || pDesklet->iBottomSurfaceOffset != 0)
	{
		cairo_translate (pCairoContext, pDesklet->iLeftSurfaceOffset, pDesklet->iTopSurfaceOffset);
		cairo_scale (pCairoContext,
			1. - 1.*(pDesklet->iLeftSurfaceOffset + pDesklet->iRightSurfaceOffset) / pDesklet->iWidth,
			1. - 1.*(pDesklet->iTopSurfaceOffset + pDesklet->iBottomSurfaceOffset) / pDesklet->iHeight);
	}
	
	if (pDesklet->pRenderer != NULL)  // un moteur de rendu specifique a ete fourni.
	{
		if (pDesklet->pRenderer->render != NULL)
			pDesklet->pRenderer->render (pCairoContext, pDesklet, bRenderOptimized);
	}
	
	cairo_restore (pCairoContext);
	if (pDesklet->pForeGroundSurface != NULL && pDesklet->fForeGroundAlpha != 0)
	{
		cairo_set_source_surface (pCairoContext,
			pDesklet->pForeGroundSurface,
			0.,
			0.);
		if (pDesklet->fForeGroundAlpha == 1)
			cairo_paint (pCairoContext);
		else
			cairo_paint_with_alpha (pCairoContext, pDesklet->fForeGroundAlpha);
	}
	
	if ((pDesklet->bInside || pDesklet->rotating) && ! pDesklet->bPositionLocked)
	{
		if (! pDesklet->rotating)
			cairo_restore (pCairoContext);
		if (s_pRotateButtonSurface != NULL)
		{
			cairo_set_source_surface (pCairoContext, s_pRotateButtonSurface, 0., 0.);
			cairo_paint (pCairoContext);
		}
		if (s_pRetachButtonSurface != NULL)
		{
			cairo_set_source_surface (pCairoContext, s_pRetachButtonSurface, pDesklet->iWidth - myDesklets.iDeskletButtonSize, 0.);
			cairo_paint (pCairoContext);
		}
	}

	cairo_destroy (pCairoContext);
}
gboolean cairo_dock_render_desklet_notification (gpointer pUserData, CairoDesklet *pDesklet)
{
	glLoadIdentity ();
	glTranslatef (pDesklet->iWidth/2, pDesklet->iHeight/2, 0.);
	
	if (pDesklet->fZoom != 1)
	{
		glScalef (pDesklet->fZoom, pDesklet->fZoom, 1.);
	}
	
	glPushMatrix ();
	if (pDesklet->fRotation != 0)
	{
		double alpha = atan2 (pDesklet->iHeight, pDesklet->iWidth);
		double theta = fabs (pDesklet->fRotation);
		if (theta > G_PI/2)
			theta -= G_PI/2;
		double fZoomX, fZoomY;
		double d = .5 * sqrt (pDesklet->iWidth * pDesklet->iWidth + pDesklet->iHeight * pDesklet->iHeight);
		fZoomX = fabs (.5 * pDesklet->iWidth / (d * sin (alpha + theta)));
		fZoomY = fabs (.5 * pDesklet->iHeight / (d * cos (alpha - theta)));
		//g_print ("d = %.2f ; alpha = %.2f ; zoom : %.2fx%.2f\n", d, alpha/G_PI*180., fZoomX, fZoomY);
		glRotatef (- pDesklet->fRotation / G_PI * 180., 0., 0., 1.);
		glScalef (fZoomX, fZoomY, 1.);
	}
	
	if (pDesklet->iBackGroundTexture != 0 && pDesklet->fBackGroundAlpha != 0)
	{
		glPushMatrix ();
		
		glEnable (GL_BLEND);
		glBlendFunc (GL_ONE, GL_ZERO);
		glColor4f (1., 1., 1., pDesklet->fBackGroundAlpha);
		
		glEnable (GL_TEXTURE_2D);
		glBindTexture (GL_TEXTURE_2D, pDesklet->iBackGroundTexture);
		//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		
		glPolygonMode (GL_FRONT, GL_FILL);
		glNormal3f (0., 0., 1.);
		glScalef (pDesklet->iWidth, pDesklet->iHeight, 1.);
		
		glBegin(GL_QUADS);
		glTexCoord2f(0., 0.); glVertex3f(-.5,  .5, 0.);  // Bottom Left Of The Texture and Quad
		glTexCoord2f(1., 0.); glVertex3f( .5,  .5, 0.);  // Bottom Right Of The Texture and Quad
		glTexCoord2f(1., 1.); glVertex3f( .5, -.5, 0.);  // Top Right Of The Texture and Quad
		glTexCoord2f(0., 1.); glVertex3f(-.5, -.5, 0.);  // Top Left Of The Texture and Quad
		glEnd();
		
		glDisable (GL_TEXTURE_2D);
		glDisable (GL_BLEND);
		glPopMatrix ();
	}
	
	glPushMatrix ();
	if (pDesklet->iLeftSurfaceOffset != 0 || pDesklet->iTopSurfaceOffset != 0 || pDesklet->iRightSurfaceOffset != 0 || pDesklet->iBottomSurfaceOffset != 0)
	{
		glTranslatef (pDesklet->iLeftSurfaceOffset - pDesklet->iRightSurfaceOffset, pDesklet->iBottomSurfaceOffset - pDesklet->iTopSurfaceOffset, 0.);
		glScalef (1. - 1.*(pDesklet->iLeftSurfaceOffset + pDesklet->iRightSurfaceOffset) / pDesklet->iWidth,
			1. - 1.*(pDesklet->iTopSurfaceOffset + pDesklet->iBottomSurfaceOffset) / pDesklet->iHeight,
			1.);
	}
	
	if (pDesklet->pRenderer != NULL)  // un moteur de rendu specifique a ete fourni.
	{
		if (pDesklet->pRenderer->render_opengl != NULL)
			pDesklet->pRenderer->render_opengl (pDesklet);
	}
	glPopMatrix ();
	
	if (pDesklet->iForeGroundTexture != 0 && pDesklet->fForeGroundAlpha != 0)
	{
		glPushMatrix ();
		glColor4f (1., 1., 1., pDesklet->fForeGroundAlpha);
		cairo_dock_draw_texture (pDesklet->iForeGroundTexture, pDesklet->iWidth, pDesklet->iHeight);
		glPopMatrix ();
	}
	
	if ((pDesklet->bInside || pDesklet->rotating) && ! pDesklet->bPositionLocked)
	{
		if (! pDesklet->rotating)
			glPopMatrix ();
		if (iRotateButtonTexture != 0)
		{
			glPushMatrix ();
			glTranslatef (-pDesklet->iWidth/2 + myDesklets.iDeskletButtonSize/2,
				pDesklet->iHeight/2 - myDesklets.iDeskletButtonSize/2,
				0.);
			glColor4f (1., 1., 1., 1.);
			cairo_dock_draw_texture (iRotateButtonTexture, myDesklets.iDeskletButtonSize, myDesklets.iDeskletButtonSize);
			glPopMatrix ();
		}
		if (iRetachButtonTexture != 0)
		{
			glPushMatrix ();
			glTranslatef (pDesklet->iWidth/2 - myDesklets.iDeskletButtonSize/2,
				pDesklet->iHeight/2 - myDesklets.iDeskletButtonSize/2,
				0.);
			glColor4f (1., 1., 1., 1.);
			cairo_dock_draw_texture (iRetachButtonTexture, myDesklets.iDeskletButtonSize, myDesklets.iDeskletButtonSize);
			glPopMatrix ();
		}
		if (pDesklet->rotating)
			glPopMatrix ();
	}
	else
		glPopMatrix ();
	
	return CAIRO_DOCK_LET_PASS_NOTIFICATION;
}
static void _cairo_dock_render_desklet_opengl (CairoDesklet *pDesklet)
{
	GdkGLContext *pGlContext = gtk_widget_get_gl_context (pDesklet->pWidget);
	GdkGLDrawable *pGlDrawable = gtk_widget_get_gl_drawable (pDesklet->pWidget);
	if (!gdk_gl_drawable_gl_begin (pGlDrawable, pGlContext))
		return ;
	
	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth (1.0f);
	
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity ();
	
	cairo_dock_notify (CAIRO_DOCK_RENDER_DESKLET, pDesklet);
	
	if (gdk_gl_drawable_is_double_buffered (pGlDrawable))
		gdk_gl_drawable_swap_buffers (pGlDrawable);
	else
		glFlush ();
	gdk_gl_drawable_gl_end (pGlDrawable);
}
static gboolean on_expose_desklet(GtkWidget *pWidget,
	GdkEventExpose *pExpose,
	CairoDesklet *pDesklet)
{
	//cd_debug ("%s ()", __func__);
	if (!pDesklet)
		return FALSE;
	
	if (pDesklet->iDesiredWidth != 0 && pDesklet->iDesiredHeight != 0 && (pDesklet->iKnownWidth != pDesklet->iDesiredWidth || pDesklet->iKnownHeight != pDesklet->iDesiredHeight))
	{
		//g_print ("on saute le dessin\n");
		if (g_bUseOpenGL)
		{
			// dessiner la fausse transparence.
		}
		else
		{
			cairo_t *pCairoContext = cairo_dock_create_drawing_context (CAIRO_CONTAINER (pDesklet));
			cairo_destroy (pCairoContext);
		}
		return FALSE;
	}
	
	pDesklet->iDesiredWidth = 0;  /// ???
	pDesklet->iDesiredHeight = 0;
		
	if (g_bUseOpenGL && pDesklet->pRenderer && pDesklet->pRenderer->render_opengl)
	{
		_cairo_dock_render_desklet_opengl (pDesklet);
	}
	else
	{
		_cairo_dock_render_desklet (pDesklet, &pExpose->area);
	}
	
	return FALSE;
}


static gboolean _cairo_dock_gl_animation (CairoDesklet *pDesklet)
{
	gboolean bContinue = FALSE;
	/*Icon *icon;
	icon = pDesklet->pIcon;
	if (icon != NULL)
		cairo_dock_notify (CAIRO_DOCK_UPDATE_ICON, icon, pDesklet, &bContinue);
	GList *ic;
	for (ic = pDesklet->icons; ic != NULL; ic = ic->next)
	{
		icon = ic->data;
		///if (icon->bOnMouseOverAnimating)
			cairo_dock_notify (CAIRO_DOCK_UPDATE_ICON, icon, pDesklet, &bContinue);
		///bContinue |= icon->bOnMouseOverAnimating;
	}*/
	
	cairo_dock_notify (CAIRO_DOCK_UPDATE_DESKLET, pDesklet, &bContinue);
	
	gtk_widget_queue_draw (pDesklet->pWidget);
	if (! bContinue)
	{
		pDesklet->iSidGLAnimation = 0;
		return FALSE;
	}
	else
		return TRUE;
}


static gboolean _cairo_dock_write_desklet_size (CairoDesklet *pDesklet)
{
	if (pDesklet->pIcon != NULL && pDesklet->pIcon->pModuleInstance != NULL)
		cairo_dock_update_conf_file (pDesklet->pIcon->pModuleInstance->cConfFilePath,
			G_TYPE_INT, "Desklet", "width", pDesklet->iWidth,
			G_TYPE_INT, "Desklet", "height", pDesklet->iHeight,
			G_TYPE_INVALID);
	pDesklet->iSidWriteSize = 0;
	pDesklet->iKnownWidth = pDesklet->iWidth;
	pDesklet->iKnownHeight = pDesklet->iHeight;
	if (((pDesklet->iDesiredWidth != 0 || pDesklet->iDesiredHeight != 0) && pDesklet->iDesiredWidth == pDesklet->iWidth && pDesklet->iDesiredHeight == pDesklet->iHeight) || (pDesklet->iDesiredWidth == 0 && pDesklet->iDesiredHeight == 0))
	{
		pDesklet->iDesiredWidth = 0;
		pDesklet->iDesiredHeight = 0;
		
		cairo_t *pCairoContext = cairo_dock_create_context_from_window (CAIRO_CONTAINER (pDesklet));
		cairo_dock_load_desklet_decorations (pDesklet, pCairoContext);
		cairo_destroy (pCairoContext);
		
		if (pDesklet->pIcon != NULL && pDesklet->pIcon->pModuleInstance != NULL)
		{
			cairo_dock_reload_module_instance (pDesklet->pIcon->pModuleInstance, FALSE);
			gtk_widget_queue_draw (pDesklet->pWidget);  // sinon on ne redessine que l'interieur.
		}
	}
	
	//g_print ("iWidth <- %d;iHeight <- %d ; (%dx%d) (%x)\n", pDesklet->iWidth, pDesklet->iHeight, pDesklet->iKnownWidth, pDesklet->iKnownHeight, pDesklet->pIcon);
	return FALSE;
}
static gboolean _cairo_dock_write_desklet_position (CairoDesklet *pDesklet)
{
	if (pDesklet->pIcon != NULL && pDesklet->pIcon->pModuleInstance != NULL)
	{
		int iRelativePositionX = (pDesklet->iWindowPositionX + pDesklet->iWidth/2 <= g_iXScreenWidth[CAIRO_DOCK_HORIZONTAL]/2 ? pDesklet->iWindowPositionX : pDesklet->iWindowPositionX - g_iXScreenWidth[CAIRO_DOCK_HORIZONTAL]);
		int iRelativePositionY = (pDesklet->iWindowPositionY + pDesklet->iHeight/2 <= g_iXScreenHeight[CAIRO_DOCK_HORIZONTAL]/2 ? pDesklet->iWindowPositionY : pDesklet->iWindowPositionY - g_iXScreenHeight[CAIRO_DOCK_HORIZONTAL]);
		cairo_dock_update_conf_file (pDesklet->pIcon->pModuleInstance->cConfFilePath,
			G_TYPE_INT, "Desklet", "x position", iRelativePositionX,
			G_TYPE_INT, "Desklet", "y position", iRelativePositionY,
			G_TYPE_INVALID);
		
		/*GdkBitmap* pShapeBitmap = (GdkBitmap*) gdk_pixmap_new (NULL, pDesklet->iWidth, pDesklet->iHeight, 1);
		cairo_t* pCairoContext = gdk_cairo_create (pShapeBitmap);
		if (cairo_status (pCairoContext) == CAIRO_STATUS_SUCCESS)
		{
			cairo_set_source_rgb (pCairoContext, 1, 1, 1);
			cairo_paint (pCairoContext);
			cairo_destroy (pCairoContext);
		
		
			gtk_widget_input_shape_combine_mask (pDesklet->pWidget,
				NULL,
				0,
				0);
			gtk_widget_input_shape_combine_mask (pDesklet->pWidget,
				pShapeBitmap,
				0,
				0);
		}
		g_object_unref ((gpointer) pShapeBitmap);*/
	}
	pDesklet->iSidWritePosition = 0;
	return FALSE;
}
static gboolean on_configure_desklet (GtkWidget* pWidget,
	GdkEventConfigure* pEvent,
	CairoDesklet *pDesklet)
{
	//cd_debug ("%s (%dx%d ; %d,%d)", __func__, pEvent->width, pEvent->height, (int) pEvent->x, (int) pEvent->y);
	if (pDesklet->iWidth != pEvent->width || pDesklet->iHeight != pEvent->height)
	{
		if ((pEvent->width < pDesklet->iWidth || pEvent->height < pDesklet->iHeight) && (pDesklet->iDesiredWidth != 0 && pDesklet->iDesiredHeight != 0))
		{
			gdk_window_resize (pDesklet->pWidget->window,
				pDesklet->iDesiredWidth,
				pDesklet->iDesiredHeight);
		}
		
		pDesklet->iWidth = pEvent->width;
		pDesklet->iHeight = pEvent->height;
		
		if (g_bUseOpenGL)
		{
			GdkGLContext* pGlContext = gtk_widget_get_gl_context (pWidget);
			GdkGLDrawable* pGlDrawable = gtk_widget_get_gl_drawable (pWidget);
			GLsizei w = pEvent->width;
			GLsizei h = pEvent->height;
			if (!gdk_gl_drawable_gl_begin (pGlDrawable, pGlContext))
				return FALSE;
			
			glViewport(0, 0, w, h);
			
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, w, 0, h, 0.0, 500.0);
			//gluPerspective(30.0, 1.0*(GLfloat)w/(GLfloat)h, 1.0, 500.0);
			
			glMatrixMode (GL_MODELVIEW);
			glLoadIdentity ();
			gluLookAt (w/2, h/2, 3.,
				w/2, h/2, 0.,
				0.0f, 1.0f, 0.0f);
			glTranslatef (0.0f, 0.0f, -3.);
			
			glClearAccum (0., 0., 0., 0.);
			glClear (GL_ACCUM_BUFFER_BIT);
			
			gdk_gl_drawable_gl_end (pGlDrawable);
		}
		
		if (pDesklet->iSidWriteSize != 0)
		{
			g_source_remove (pDesklet->iSidWriteSize);
		}
		pDesklet->iSidWriteSize = g_timeout_add (500, (GSourceFunc) _cairo_dock_write_desklet_size, (gpointer) pDesklet);
	}

	if (pDesklet->iWindowPositionX != pEvent->x || pDesklet->iWindowPositionY != pEvent->y)
	{
		pDesklet->iWindowPositionX = pEvent->x;
		pDesklet->iWindowPositionY = pEvent->y;

		if (pDesklet->iSidWritePosition != 0)
		{
			g_source_remove (pDesklet->iSidWritePosition);
		}
		pDesklet->iSidWritePosition = g_timeout_add (500, (GSourceFunc) _cairo_dock_write_desklet_position, (gpointer) pDesklet);
	}
	pDesklet->moving = FALSE;

	return FALSE;
}

gboolean on_scroll_desklet (GtkWidget* pWidget,
	GdkEventScroll* pScroll,
	CairoDesklet *pDesklet)
{
	if (pScroll->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
	{
		Icon *icon = cairo_dock_find_clicked_icon_in_desklet (pDesklet);
		if (icon != NULL)
		{
			cairo_dock_notify (CAIRO_DOCK_SCROLL_ICON, icon, pDesklet, pScroll->direction);
		}
	}
	return FALSE;
}


Icon *cairo_dock_find_clicked_icon_in_desklet (CairoDesklet *pDesklet)
{
	int iMouseX = pDesklet->iMouseX;
	int iMouseY = pDesklet->iMouseY;
	cd_debug (" clic en (%d;%d)", iMouseX, iMouseY);
	
	Icon *icon = pDesklet->pIcon;
	g_return_val_if_fail (icon != NULL, NULL);  // peut arriver au tout debut, car on associe l'icone au desklet _apres_ l'avoir cree, et on fait tourner la gtk_main entre-temps (pour le redessiner invisible).
	if (icon->fDrawX < iMouseX && icon->fDrawX + icon->fWidth * icon->fScale > iMouseX && icon->fDrawY < iMouseY && icon->fDrawY + icon->fHeight * icon->fScale > iMouseY)
	{
		return icon;
	}
	
	if (pDesklet->icons != NULL)
	{
		GList* ic;
		for (ic = pDesklet->icons; ic != NULL; ic = ic->next)
		{
			icon = ic->data;
			if (icon->fDrawX < iMouseX && icon->fDrawX + icon->fWidth * icon->fScale > iMouseX && icon->fDrawY < iMouseY && icon->fDrawY + icon->fHeight * icon->fScale > iMouseY)
			{
				return icon;
			}
		}
	}
	return NULL;
}

static gboolean on_button_press_desklet(GtkWidget *pWidget,
	GdkEventButton *pButton,
	CairoDesklet *pDesklet)
{
	if (pButton->button == 1)  // clic gauche.
	{
		if (pButton->type == GDK_BUTTON_PRESS)
		{
			pDesklet->iMouseX = pButton->x;  // pour le deplacement manuel.
			pDesklet->iMouseY = pButton->y;
			
			if (pButton->x < myDesklets.iDeskletButtonSize && pButton->y < myDesklets.iDeskletButtonSize)
				pDesklet->rotating = TRUE;
			else if (pButton->x > pDesklet->iWidth - myDesklets.iDeskletButtonSize && pButton->y < myDesklets.iDeskletButtonSize)
			{
				pDesklet->retaching = TRUE;
			}
			else
				pDesklet->time = pButton->time;
		}
		else if (pButton->type == GDK_BUTTON_RELEASE)
		{
			cd_debug ("GDK_BUTTON_RELEASE");
			if (pDesklet->moving)
			{
				pDesklet->moving = FALSE;
			}
			else if (pDesklet->rotating)
			{
				pDesklet->rotating = FALSE;
				cairo_dock_update_conf_file (pDesklet->pIcon->pModuleInstance->cConfFilePath,
					G_TYPE_INT, "Desklet", "rotation", (int) (pDesklet->fRotation / G_PI * 180.),
					G_TYPE_INVALID);
				gtk_widget_queue_draw (pDesklet->pWidget);
			}
			else if (pDesklet->retaching)
			{
				pDesklet->retaching = FALSE;
				if (! pDesklet->bPositionLocked && pButton->x > pDesklet->iWidth - myDesklets.iDeskletButtonSize && pButton->y < myDesklets.iDeskletButtonSize)
				{
					Icon *icon = pDesklet->pIcon;
					g_return_val_if_fail (CAIRO_DOCK_IS_APPLET (icon), FALSE);
					cairo_dock_update_conf_file (icon->pModuleInstance->cConfFilePath,
						G_TYPE_BOOLEAN, "Desklet", "initially detached", FALSE,
						G_TYPE_INVALID);
					cairo_dock_reload_module_instance (icon->pModuleInstance, TRUE);
					return FALSE;
				}
			}
			else
			{
				Icon *pClickedIcon = cairo_dock_find_clicked_icon_in_desklet (pDesklet);
				cairo_dock_notify (CAIRO_DOCK_CLICK_ICON, pClickedIcon, pDesklet, pButton->state);
			}
		}
		else if (pButton->type == GDK_2BUTTON_PRESS)
		{
			if (pButton->x < myDesklets.iDeskletButtonSize && pButton->y < myDesklets.iDeskletButtonSize)
			{
				pDesklet->fRotation = 0.;
				gtk_widget_queue_draw (pDesklet->pWidget);
				cairo_dock_update_conf_file (pDesklet->pIcon->pModuleInstance->cConfFilePath,
					G_TYPE_INT, "Desklet", "rotation", 0,
					G_TYPE_INVALID);
			}
			else
			{
				Icon *pClickedIcon = cairo_dock_find_clicked_icon_in_desklet (pDesklet);
				cairo_dock_notify (CAIRO_DOCK_DOUBLE_CLICK_ICON, pClickedIcon, pDesklet);
			}
		}
	}
	else if (pButton->button == 3 && pButton->type == GDK_BUTTON_PRESS)  // clique droit.
	{
		Icon *pClickedIcon = cairo_dock_find_clicked_icon_in_desklet (pDesklet);
		GtkWidget *menu = cairo_dock_build_menu (pClickedIcon, CAIRO_CONTAINER (pDesklet));  // genere un CAIRO_DOCK_BUILD_MENU.
		gtk_widget_show_all (menu);
		gtk_menu_popup (GTK_MENU (menu),
			NULL,
			NULL,
			NULL,
			NULL,
			1,
			gtk_get_current_event_time ());
		pDesklet->bInside = FALSE;
		pDesklet->iGradationCount = 0;  // on force le fond a redevenir transparent.
		gtk_widget_queue_draw (pDesklet->pWidget);
	}
	else if (pButton->button == 2 && pButton->type == GDK_BUTTON_PRESS)  // clique milieu.
	{
		if (pButton->x < myDesklets.iDeskletButtonSize && pButton->y < myDesklets.iDeskletButtonSize)
		{
			pDesklet->fRotation = 0.;
			gtk_widget_queue_draw (pDesklet->pWidget);
			cairo_dock_update_conf_file (pDesklet->pIcon->pModuleInstance->cConfFilePath,
				G_TYPE_INT, "Desklet", "rotation", 0,
				G_TYPE_INVALID);
		}
		else
		{
			cairo_dock_notify (CAIRO_DOCK_MIDDLE_CLICK_ICON, pDesklet->pIcon, pDesklet);
		}
	}
	return FALSE;
}

void on_drag_data_received_desklet (GtkWidget *pWidget, GdkDragContext *dc, gint x, gint y, GtkSelectionData *selection_data, guint info, guint t, CairoDesklet *pDesklet)
{
	//g_print ("%s (%dx%d)\n", __func__, x, y);
	
	//\_________________ On recupere l'URI.
	gchar *cReceivedData = (gchar *) selection_data->data;
	g_return_if_fail (cReceivedData != NULL);
	
	pDesklet->iMouseX = x;
	pDesklet->iMouseY = y;
	Icon *pClickedIcon = cairo_dock_find_clicked_icon_in_desklet (pDesklet);
	cairo_dock_notify_drop_data (cReceivedData, pClickedIcon, 0, CAIRO_CONTAINER (pDesklet));
}

static gboolean on_motion_notify_desklet(GtkWidget *pWidget,
	GdkEventMotion* pMotion,
	CairoDesklet *pDesklet)
{
	if (pMotion->state & GDK_BUTTON1_MASK && ! pDesklet->bPositionLocked)
	{
		cd_debug ("root : %d;%d", (int) pMotion->x_root, (int) pMotion->y_root);
		/*pDesklet->moving = TRUE;
		gtk_window_move (GTK_WINDOW (pWidget),
			pMotion->x_root + pDesklet->diff_x,
			pMotion->y_root + pDesklet->diff_y);*/
	}
	else  // le 'press-button' est local au sous-widget clique, alors que le 'motion-notify' est global a la fenetre; c'est donc par lui qu'on peut avoir a coup sur les coordonnees du curseur (juste avant le clic).
	{
		pDesklet->iMouseX = pMotion->x;
		pDesklet->iMouseY = pMotion->y;
		gboolean bStartAnimation = FALSE;
		cairo_dock_notify (CAIRO_DOCK_MOUSE_MOVED, pDesklet, &bStartAnimation);
		if (bStartAnimation && pDesklet->iSidGLAnimation == 0)
			pDesklet->iSidGLAnimation = g_timeout_add (mySystem.iGLAnimationDeltaT, (GSourceFunc) _cairo_dock_gl_animation, pDesklet);
	}
	
	if (pDesklet->rotating && ! pDesklet->bPositionLocked)
	{
		double alpha = atan2 (pDesklet->iHeight, - pDesklet->iWidth);
		pDesklet->fRotation = alpha - atan2 (.5*pDesklet->iHeight - pMotion->y, pMotion->x - .5*pDesklet->iWidth);
		while (pDesklet->fRotation > G_PI)
			pDesklet->fRotation -= 2 * G_PI;
		while (pDesklet->fRotation <= - G_PI)
			pDesklet->fRotation += 2 * G_PI;
		gtk_widget_queue_draw(pDesklet->pWidget);
	}
	else if (pMotion->state & GDK_BUTTON1_MASK && ! pDesklet->bPositionLocked && ! pDesklet->moving)
	{
		gtk_window_begin_move_drag (GTK_WINDOW (gtk_widget_get_toplevel (pWidget)),
			1/*pButton->button*/,
			pMotion->x_root/*pButton->x_root*/,
			pMotion->y_root/*pButton->y_root*/,
			pDesklet->time/*pButton->time*/);
		pDesklet->moving = TRUE;
	}
	gdk_device_get_state (pMotion->device, pMotion->window, NULL, NULL);  // pour recevoir d'autres MotionNotify.
	return FALSE;
}


static gboolean cd_desklet_on_focus_in_out(GtkWidget *widget,
	GdkEventFocus *event,
	CairoDesklet *pDesklet)
{
	if (pDesklet)
		gtk_widget_queue_draw(pDesklet->pWidget);
	return FALSE;
}

static gboolean _cairo_dock_desklet_gradation (CairoDesklet *pDesklet)
{
	pDesklet->iGradationCount += (pDesklet->bInside ? 1 : -1);
	gtk_widget_queue_draw (pDesklet->pWidget);
	
	if (pDesklet->iGradationCount <= 0 || pDesklet->iGradationCount >= CD_NB_ITER_FOR_GRADUATION)
	{
		if (pDesklet->iGradationCount < 0)
			pDesklet->iGradationCount = 0;
		else if (pDesklet->iGradationCount > CD_NB_ITER_FOR_GRADUATION)
			pDesklet->iGradationCount = CD_NB_ITER_FOR_GRADUATION;
		pDesklet->iSidGradationOnEnter = 0;
		return FALSE;
	}
	return TRUE;
}
static gboolean on_enter_desklet (GtkWidget* pWidget,
	GdkEventCrossing* pEvent,
	CairoDesklet *pDesklet)
{
	//g_print ("%s (%d)\n", __func__, pDesklet->bInside);
	if (! pDesklet->bInside)  // avant on etait dehors, on redessine donc.
	{
		pDesklet->bInside = TRUE;
		if (pDesklet->iSidGradationOnEnter == 0)
		{
			pDesklet->iSidGradationOnEnter = g_timeout_add (50, (GSourceFunc) _cairo_dock_desklet_gradation, (gpointer) pDesklet);
		}
		
		if (g_bUseOpenGL/* && pDesklet->pRenderer && pDesklet->pRenderer->render_opengl != NULL*/)
		{
			gboolean bStartAnimation = FALSE;
			cairo_dock_notify (CAIRO_DOCK_ENTER_DESKLET, pDesklet, &bStartAnimation);
			if (bStartAnimation && pDesklet->iSidGLAnimation == 0)
			{
				pDesklet->iSidGLAnimation = g_timeout_add (mySystem.iGLAnimationDeltaT, (GSourceFunc)_cairo_dock_gl_animation, pDesklet);
			}
		}
	}
	return FALSE;
}
static gboolean on_leave_desklet (GtkWidget* pWidget,
	GdkEventCrossing* pEvent,
	CairoDesklet *pDesklet)
{
	//g_print ("%s (%d)\n", __func__, pDesklet->bInside);
	int iMouseX, iMouseY;
	gdk_window_get_pointer (pWidget->window, &iMouseX, &iMouseY, NULL);
	if (gtk_bin_get_child (GTK_BIN (pDesklet->pWidget)) != NULL && iMouseX > 0 && iMouseX < pDesklet->iWidth && iMouseY > 0 && iMouseY < pDesklet->iHeight)  // en fait on est dans un widget fils, donc on ne fait rien.
	{
		return FALSE;
	}

	pDesklet->bInside = FALSE;
	if (pDesklet->iSidGradationOnEnter == 0)
	{
		pDesklet->iSidGradationOnEnter = g_timeout_add (50, (GSourceFunc) _cairo_dock_desklet_gradation, (gpointer) pDesklet);
	}
	return FALSE;
}

gboolean on_delete_desklet (GtkWidget *pWidget, GdkEvent *event, CairoDesklet *pDesklet)
{
	if (pDesklet->pIcon->pModuleInstance != NULL)
	{
		cairo_dock_update_conf_file (pDesklet->pIcon->pModuleInstance->cConfFilePath,
			G_TYPE_BOOLEAN, "Desklet", "initially detached", FALSE,
			G_TYPE_INVALID);

		cairo_dock_reload_module_instance (pDesklet->pIcon->pModuleInstance, TRUE);
	}
	return TRUE;
}


CairoDesklet *cairo_dock_create_desklet (Icon *pIcon, GtkWidget *pInteractiveWidget, gboolean bOnWidgetLayer)
{
	cd_message ("%s ()", __func__);
	CairoDesklet *pDesklet = g_new0(CairoDesklet, 1);
	pDesklet->iType = CAIRO_DOCK_TYPE_DESKLET;
	pDesklet->bIsHorizontal = TRUE;
	pDesklet->bDirectionUp = TRUE;
	pDesklet->fZoom = 1;
	
	GtkWidget* pWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	if (bOnWidgetLayer)
		gtk_window_set_type_hint (GTK_WINDOW (pWindow), GDK_WINDOW_TYPE_HINT_UTILITY);
	pDesklet->pWidget = pWindow;
	pDesklet->pIcon = pIcon;
	
	if (g_bSticky)
		gtk_window_stick(GTK_WINDOW(pWindow));
	gtk_window_set_skip_pager_hint(GTK_WINDOW(pWindow), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(pWindow), TRUE);
	
	cairo_dock_set_colormap_for_window(pWindow);
	if (g_bUseOpenGL)
	{
		GdkGLContext *pMainGlContext = gtk_widget_get_gl_context (g_pMainDock->pWidget);
		gtk_widget_set_gl_capability (pWindow,
			g_pGlConfig,
			pMainGlContext,  // on partage les ressources entre les contextes.
			! g_bIndirectRendering,  // TRUE <=> direct connection to the graphics system.
			GDK_GL_RGBA_TYPE);
	}
	gtk_widget_set_app_paintable(pWindow, TRUE);
	gtk_window_set_decorated(GTK_WINDOW(pWindow), FALSE);
	gtk_window_set_resizable(GTK_WINDOW(pWindow), TRUE);
	gtk_window_set_title(GTK_WINDOW(pWindow), "cairo-dock-desklet");  /// distinguer titre et classe ?...
	gtk_widget_add_events(pWindow, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_FOCUS_CHANGE_MASK);
	//the border is were cairo paint
	gtk_container_set_border_width(GTK_CONTAINER(pWindow), myBackground.iDockRadius/2);  /// re-utiliser la formule des dialogues...
	gtk_window_set_default_size(GTK_WINDOW(pWindow), 2*myBackground.iDockRadius+1, 2*myBackground.iDockRadius+1);

	g_signal_connect (G_OBJECT (pWindow),
		"expose-event",
		G_CALLBACK (on_expose_desklet),
		pDesklet);
	g_signal_connect (G_OBJECT (pWindow),
		"configure-event",
		G_CALLBACK (on_configure_desklet),
		pDesklet);
	g_signal_connect (G_OBJECT (pWindow),
		"motion-notify-event",
		G_CALLBACK (on_motion_notify_desklet),
		pDesklet);
	g_signal_connect (G_OBJECT (pWindow),
		"button-press-event",
		G_CALLBACK (on_button_press_desklet),
		pDesklet);
	g_signal_connect (G_OBJECT (pWindow),
		"button-release-event",
		G_CALLBACK (on_button_press_desklet),
		pDesklet);
	g_signal_connect (G_OBJECT (pWindow),
		"focus-in-event",
		G_CALLBACK (cd_desklet_on_focus_in_out),
		pDesklet);
	g_signal_connect (G_OBJECT (pWindow),
		"focus-out-event",
		G_CALLBACK (cd_desklet_on_focus_in_out),
		pDesklet);
	g_signal_connect (G_OBJECT (pWindow),
		"enter-notify-event",
		G_CALLBACK (on_enter_desklet),
		pDesklet);
	g_signal_connect (G_OBJECT (pWindow),
		"leave-notify-event",
		G_CALLBACK (on_leave_desklet),
		pDesklet);
	g_signal_connect (G_OBJECT (pWindow),
		"delete-event",
		G_CALLBACK (on_delete_desklet),
		pDesklet);
	g_signal_connect (G_OBJECT (pWindow),
		"scroll-event",
		G_CALLBACK (on_scroll_desklet),
		pDesklet);
	cairo_dock_allow_widget_to_receive_data (pWindow, G_CALLBACK (on_drag_data_received_desklet), pDesklet);

  //user widget
  if (pInteractiveWidget != NULL)
  {
    cd_debug ("ref = %d", pInteractiveWidget->object.parent_instance.ref_count);
    gtk_container_add (GTK_CONTAINER (pDesklet->pWidget), pInteractiveWidget);
    cd_debug ("pack -> ref = %d", pInteractiveWidget->object.parent_instance.ref_count);
  }

  gtk_widget_show_all(pWindow);

  return pDesklet;
}

void cairo_dock_configure_desklet (CairoDesklet *pDesklet, CairoDockMinimalAppletConfig *pMinimalConfig)
{
	cd_debug ("%s (%dx%d ; (%d,%d) ; %d,%d,%d)", __func__, pMinimalConfig->iDeskletWidth, pMinimalConfig->iDeskletHeight, pMinimalConfig->iDeskletPositionX, pMinimalConfig->iDeskletPositionY, pMinimalConfig->bKeepBelow, pMinimalConfig->bKeepAbove, pMinimalConfig->bOnWidgetLayer);
	if (pMinimalConfig->bDeskletUseSize && (pMinimalConfig->iDeskletWidth != pDesklet->iWidth || pMinimalConfig->iDeskletHeight != pDesklet->iHeight))
	{
		pDesklet->iDesiredWidth = pMinimalConfig->iDeskletWidth;
		pDesklet->iDesiredHeight = pMinimalConfig->iDeskletHeight;
		gdk_window_resize (pDesklet->pWidget->window,
			pMinimalConfig->iDeskletWidth,
			pMinimalConfig->iDeskletHeight);
	}
	
	int iAbsolutePositionX = (pMinimalConfig->iDeskletPositionX < 0 ? g_iXScreenWidth[CAIRO_DOCK_HORIZONTAL] + pMinimalConfig->iDeskletPositionX : pMinimalConfig->iDeskletPositionX);
	iAbsolutePositionX = MAX (0, MIN (g_iXScreenWidth[CAIRO_DOCK_HORIZONTAL] - pMinimalConfig->iDeskletWidth, iAbsolutePositionX));
	int iAbsolutePositionY = (pMinimalConfig->iDeskletPositionY < 0 ? g_iXScreenHeight[CAIRO_DOCK_HORIZONTAL] + pMinimalConfig->iDeskletPositionY : pMinimalConfig->iDeskletPositionY);
	iAbsolutePositionY = MAX (0, MIN (g_iXScreenHeight[CAIRO_DOCK_HORIZONTAL] - pMinimalConfig->iDeskletHeight, iAbsolutePositionY));
	
	gdk_window_move(pDesklet->pWidget->window,
		iAbsolutePositionX,
		iAbsolutePositionY);
	cd_debug (" -> (%d;%d)", iAbsolutePositionX, iAbsolutePositionY);

	gtk_window_set_keep_below (GTK_WINDOW (pDesklet->pWidget), pMinimalConfig->bKeepBelow);
	gtk_window_set_keep_above (GTK_WINDOW (pDesklet->pWidget), pMinimalConfig->bKeepAbove);

	Window Xid = GDK_WINDOW_XID (pDesklet->pWidget->window);
	if (pMinimalConfig->bOnWidgetLayer)
		cairo_dock_set_xwindow_type_hint (Xid, "_NET_WM_WINDOW_TYPE_UTILITY");  // le hide-show le fait deconner completement, il perd son skip_task_bar ! au moins sous KDE.
	else
		cairo_dock_set_xwindow_type_hint (Xid, "_NET_WM_WINDOW_TYPE_NORMAL");
	
	pDesklet->bPositionLocked = pMinimalConfig->bPositionLocked;
	pDesklet->fRotation = pMinimalConfig->iRotation / 180. * G_PI ;
	
	g_free (pDesklet->cDecorationTheme);
	pDesklet->cDecorationTheme = pMinimalConfig->cDecorationTheme;
	pMinimalConfig->cDecorationTheme = NULL;
	cairo_dock_free_desklet_decoration (pDesklet->pUserDecoration);
	pDesklet->pUserDecoration = pMinimalConfig->pUserDecoration;
	pMinimalConfig->pUserDecoration = NULL;
	
	cd_debug ("%s (%dx%d ; %d)", __func__, pDesklet->iDesiredWidth, pDesklet->iDesiredHeight, pDesklet->iSidWriteSize);
	if (pDesklet->iDesiredWidth == 0 && pDesklet->iDesiredHeight == 0 && pDesklet->iSidWriteSize == 0)
	{
		cairo_t *pCairoContext = cairo_dock_create_context_from_window (CAIRO_CONTAINER (pDesklet));
		cairo_dock_load_desklet_decorations (pDesklet, pCairoContext);
		cairo_destroy (pCairoContext);
	}
}


void cairo_dock_steal_interactive_widget_from_desklet (CairoDesklet *pDesklet)
{
	if (pDesklet == NULL)
		return;

	GtkWidget *pInteractiveWidget = gtk_bin_get_child (GTK_BIN (pDesklet->pWidget));
	if (pInteractiveWidget != NULL)
		cairo_dock_steal_widget_from_its_container (pInteractiveWidget);
}

void cairo_dock_free_desklet (CairoDesklet *pDesklet)
{
	if (pDesklet == NULL)
		return;

	if (pDesklet->iSidWriteSize != 0)
		g_source_remove (pDesklet->iSidWriteSize);
	if (pDesklet->iSidWritePosition != 0)
		g_source_remove (pDesklet->iSidWritePosition);
	if (pDesklet->iSidGrowUp != 0)
		g_source_remove (pDesklet->iSidGrowUp);
	cairo_dock_notify (CAIRO_DOCK_STOP_DESKLET, pDesklet);
	if (pDesklet->iSidGLAnimation != 0)
		g_source_remove (pDesklet->iSidGLAnimation);
	
	cairo_dock_steal_interactive_widget_from_desklet (pDesklet);

	gtk_widget_destroy (pDesklet->pWidget);
	pDesklet->pWidget = NULL;
	
	if (pDesklet->pRenderer != NULL)
	{
		if (pDesklet->pRenderer->free_data != NULL)
		{
			pDesklet->pRenderer->free_data (pDesklet);
			pDesklet->pRendererData = NULL;
		}
	}
	
	if (pDesklet->icons != NULL)
	{
		g_list_foreach (pDesklet->icons, (GFunc) cairo_dock_free_icon, NULL);
		g_list_free (pDesklet->icons);
		pDesklet->icons = NULL;
	}
	
	g_free (pDesklet->cDecorationTheme);
	cairo_dock_free_desklet_decoration (pDesklet->pUserDecoration);
	
	g_free(pDesklet);
}

void cairo_dock_hide_desklet (CairoDesklet *pDesklet)
{
	if (pDesklet)
		gtk_widget_hide (pDesklet->pWidget);
}

void cairo_dock_show_desklet (CairoDesklet *pDesklet)
{
	if (pDesklet)
		gtk_window_present(GTK_WINDOW(pDesklet->pWidget));
}


void cairo_dock_add_interactive_widget_to_desklet (GtkWidget *pInteractiveWidget, CairoDesklet *pDesklet)
{
	g_return_if_fail (pDesklet != NULL);
	gtk_container_add (GTK_CONTAINER (pDesklet->pWidget), pInteractiveWidget);
}



static gboolean _cairo_dock_set_one_desklet_visible (CairoDesklet *pDesklet, CairoDockModuleInstance *pInstance, gpointer data)
{
	gboolean bOnWidgetLayerToo = GPOINTER_TO_INT (data);
	Window Xid = GDK_WINDOW_XID (pDesklet->pWidget->window);
	gboolean bIsOnWidgetLayer = cairo_dock_window_is_utility (Xid);
	if (bOnWidgetLayerToo || ! bIsOnWidgetLayer)
	{
		cd_debug ("%s (%d)", __func__, Xid);
		
		if (bIsOnWidgetLayer)  // on le passe sur la couche visible.
			cairo_dock_set_xwindow_type_hint (Xid, "_NET_WM_WINDOW_TYPE_NORMAL");
		
		gtk_window_set_keep_below (GTK_WINDOW (pDesklet->pWidget), FALSE);
		
		cairo_dock_show_desklet (pDesklet);
	}
	return FALSE;
}
void cairo_dock_set_all_desklets_visible (gboolean bOnWidgetLayerToo)
{
	cd_debug ("%s (%d)", __func__, bOnWidgetLayerToo);
	cairo_dock_foreach_desklet (_cairo_dock_set_one_desklet_visible, GINT_TO_POINTER (bOnWidgetLayerToo));
}

static gboolean _cairo_dock_set_one_desklet_visibility_to_default (CairoDesklet *pDesklet, CairoDockModuleInstance *pInstance, CairoDockMinimalAppletConfig *pMinimalConfig)
{
	GKeyFile *pKeyFile = cairo_dock_pre_read_module_instance_config (pInstance, pMinimalConfig);
	g_key_file_free (pKeyFile);
	
	gtk_window_set_keep_below (GTK_WINDOW (pDesklet->pWidget), pMinimalConfig->bKeepBelow);
	gtk_window_set_keep_above (GTK_WINDOW (pDesklet->pWidget), pMinimalConfig->bKeepAbove);
	
	Window Xid = GDK_WINDOW_XID (pDesklet->pWidget->window);
	if (pMinimalConfig->bOnWidgetLayer)
		cairo_dock_set_xwindow_type_hint (Xid, "_NET_WM_WINDOW_TYPE_UTILITY");
	else
		cairo_dock_set_xwindow_type_hint (Xid, "_NET_WM_WINDOW_TYPE_NORMAL");
	return FALSE;
}
void cairo_dock_set_desklets_visibility_to_default (void)
{
	CairoDockMinimalAppletConfig pMinimalConfig;
	cairo_dock_foreach_desklet ((CairoDockForeachDeskletFunc) _cairo_dock_set_one_desklet_visibility_to_default, &pMinimalConfig);
}

static gboolean _cairo_dock_test_one_desklet_Xid (CairoDesklet *pDesklet, CairoDockModuleInstance *pInstance, Window *pXid)
{
	return (GDK_WINDOW_XID (pDesklet->pWidget->window) == *pXid);
}
CairoDesklet *cairo_dock_get_desklet_by_Xid (Window Xid)
{
	CairoDockModuleInstance *pInstance = cairo_dock_foreach_desklet ((CairoDockForeachDeskletFunc) _cairo_dock_test_one_desklet_Xid, &Xid);
	return (pInstance != NULL ? pInstance->pDesklet : NULL);
}


static gboolean _cairo_dock_grow_up_desklet (CairoDesklet *pDesklet)
{
	pDesklet->fZoom += .1;
	gtk_widget_queue_draw (pDesklet->pWidget);
	
	if (pDesklet->fZoom >= 1.11)  // la derniere est a x1.1
	{
		pDesklet->fZoom = 1;
		pDesklet->iSidGrowUp = 0;
		return FALSE;
	}
	return TRUE;
}
void cairo_dock_zoom_out_desklet (CairoDesklet *pDesklet)
{
	g_return_if_fail (pDesklet != NULL);
	pDesklet->fZoom = 0;
	pDesklet->iSidGrowUp = g_timeout_add (50, (GSourceFunc) _cairo_dock_grow_up_desklet, (gpointer) pDesklet);
}


void cairo_dock_load_desklet_decorations (CairoDesklet *pDesklet, cairo_t *pSourceContext)
{
	if (pDesklet->pBackGroundSurface != NULL)
	{
		cairo_surface_destroy (pDesklet->pBackGroundSurface);
		pDesklet->pBackGroundSurface = NULL;
	}
	if (pDesklet->pForeGroundSurface != NULL)
	{
		cairo_surface_destroy (pDesklet->pForeGroundSurface);
		pDesklet->pForeGroundSurface = NULL;
	}
	if (pDesklet->iBackGroundTexture != 0)
	{
		glDeleteTextures (1, &pDesklet->iBackGroundTexture);
		pDesklet->iBackGroundTexture = 0;
	}
	if (pDesklet->iForeGroundTexture != 0)
	{
		glDeleteTextures (1, &pDesklet->iForeGroundTexture);
		pDesklet->iForeGroundTexture = 0;
	}
	
	CairoDeskletDecoration *pDeskletDecorations;
	cd_debug ("%s (%s)", __func__, pDesklet->cDecorationTheme);
	if (pDesklet->cDecorationTheme == NULL || strcmp (pDesklet->cDecorationTheme, "personnal") == 0)
		pDeskletDecorations = pDesklet->pUserDecoration;
	else if (strcmp (pDesklet->cDecorationTheme, "default") == 0)
		pDeskletDecorations = cairo_dock_get_desklet_decoration (myDesklets.cDeskletDecorationsName);
	else
		pDeskletDecorations = cairo_dock_get_desklet_decoration (pDesklet->cDecorationTheme);
	if (pDeskletDecorations == NULL)  // peut arriver si rendering n'a pas encore charge ses decorations.
		return ;
	cd_debug ("pDeskletDecorations : %s (%x)", pDesklet->cDecorationTheme, pDeskletDecorations);
	
	double fZoomX = 0., fZoomY = 0.;
	if  (pDeskletDecorations->cBackGroundImagePath != NULL && pDeskletDecorations->fBackGroundAlpha > 0)
	{
		cd_debug ("bg : %s", pDeskletDecorations->cBackGroundImagePath);
		pDesklet->pBackGroundSurface = cairo_dock_create_surface_from_image (pDeskletDecorations->cBackGroundImagePath,
			pSourceContext,
			1.,  // cairo_dock_get_max_scale (pDesklet)
			pDesklet->iWidth, pDesklet->iHeight,
			pDeskletDecorations->iLoadingModifier,
			&pDesklet->fImageWidth, &pDesklet->fImageHeight,
			&fZoomX, &fZoomY);
	}
	if (pDeskletDecorations->cForeGroundImagePath != NULL && pDeskletDecorations->fForeGroundAlpha > 0)
	{
		cd_debug ("fg : %s", pDeskletDecorations->cForeGroundImagePath);
		pDesklet->pForeGroundSurface = cairo_dock_create_surface_from_image (pDeskletDecorations->cForeGroundImagePath,
			pSourceContext,
			1.,  // cairo_dock_get_max_scale (pDesklet)
			pDesklet->iWidth, pDesklet->iHeight,
			pDeskletDecorations->iLoadingModifier,
			&pDesklet->fImageWidth, &pDesklet->fImageHeight,
			&fZoomX, &fZoomY);
	}
	cd_debug ("image : %.2fx%.2f ; zoom : %.2fx%.2f", pDesklet->fImageWidth, pDesklet->fImageHeight, fZoomX, fZoomY);
	pDesklet->iLeftSurfaceOffset = pDeskletDecorations->iLeftMargin * fZoomX;
	pDesklet->iTopSurfaceOffset = pDeskletDecorations->iTopMargin * fZoomY;
	pDesklet->iRightSurfaceOffset = pDeskletDecorations->iRightMargin * fZoomX;
	pDesklet->iBottomSurfaceOffset = pDeskletDecorations->iBottomMargin * fZoomY;
	pDesklet->fBackGroundAlpha = pDeskletDecorations->fBackGroundAlpha;
	pDesklet->fForeGroundAlpha = pDeskletDecorations->fForeGroundAlpha;
	cd_debug ("%d;%d;%d;%d ; %.2f;%.2f", pDesklet->iLeftSurfaceOffset, pDesklet->iTopSurfaceOffset, pDesklet->iRightSurfaceOffset, pDesklet->iBottomSurfaceOffset, pDesklet->fBackGroundAlpha, pDesklet->fForeGroundAlpha);
	if (g_bUseOpenGL)
	{
		if (pDesklet->pBackGroundSurface != NULL)
			pDesklet->iBackGroundTexture = cairo_dock_create_texture_from_surface (pDesklet->pBackGroundSurface);
		if (pDesklet->pForeGroundSurface != NULL)
			pDesklet->iForeGroundTexture = cairo_dock_create_texture_from_surface (pDesklet->pForeGroundSurface);
	}
}


void cairo_dock_free_minimal_config (CairoDockMinimalAppletConfig *pMinimalConfig)
{
	if (pMinimalConfig == NULL)
		return;
	g_free (pMinimalConfig->cLabel);
	g_free (pMinimalConfig->cIconFileName);
	g_free (pMinimalConfig->cDockName);
	g_free (pMinimalConfig->cDecorationTheme);
	cairo_dock_free_desklet_decoration (pMinimalConfig->pUserDecoration);
	g_free (pMinimalConfig);
}


static gboolean _cairo_dock_reload_one_desklet_decorations (CairoDesklet *pDesklet, CairoDockModuleInstance *pInstance, gpointer *data)
{
	gboolean bDefaultThemeOnly = GPOINTER_TO_INT (data[0]);
	cairo_t *pSourceContext = data[1];
	
	if (bDefaultThemeOnly)
	{
		if (pDesklet->cDecorationTheme == NULL || strcmp (pDesklet->cDecorationTheme, "default") == 0)
		{
			cd_debug ("on recharge les decorations de ce desklet");
			cairo_dock_load_desklet_decorations (pDesklet, pSourceContext);
		}
	}
	else  // tous ceux qui ne sont pas encore charges et qui ont leur taille definitive.
	{
		cd_debug ("pouet %dx%d ; %d ; %x;%x", pDesklet->iDesiredWidth, pDesklet->iDesiredHeight, pDesklet->iSidWriteSize, pDesklet->pBackGroundSurface, pDesklet->pForeGroundSurface);
		if (pDesklet->iDesiredWidth == 0 && pDesklet->iDesiredHeight == 0 && pDesklet->iSidWriteSize == 0 && pDesklet->pBackGroundSurface == NULL && pDesklet->pForeGroundSurface == NULL)
		{
			cd_debug ("ce desklet a saute le chargement de ses deco => on l'aide.");
			cairo_dock_load_desklet_decorations (pDesklet, pSourceContext);
		}
	}
	return FALSE;
}
void cairo_dock_reload_desklets_decorations (gboolean bDefaultThemeOnly, cairo_t *pSourceContext)  // tous ou bien seulement ceux avec "default".
{
	cd_message ("%s (%d)", __func__, bDefaultThemeOnly);
	gpointer data[2] = {GINT_TO_POINTER (bDefaultThemeOnly), pSourceContext};
	cairo_dock_foreach_desklet ((CairoDockForeachDeskletFunc)_cairo_dock_reload_one_desklet_decorations, data);
}