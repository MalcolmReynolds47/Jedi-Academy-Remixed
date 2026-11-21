/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "tr_local.h"

#define MAX_RENDER_SURFACES JAR_GHOUL2_RENDER_SURFACES
static CRenderableSurface RSStorage[MAX_RENDER_SURFACES];
static unsigned int NextRS=0;
static unsigned long long jarSurfaceDrops=0;
static unsigned int jarSurfaceFrames=0;

CRenderableSurface *AllocRS()
{
	// Submitted draw surfaces retain these pointers until the frame ends.
	// Never wrap and overwrite a different actor's mesh or bone cache.
	if (NextRS >= MAX_RENDER_SURFACES) {
		if(++jarSurfaceDrops==1 && ri.JAR_Log) ri.JAR_Log(1,"capacity","ghoul2_surfaces_full","first drop in reporting interval");
		return nullptr;
	}
	CRenderableSurface *ret=&RSStorage[NextRS];
	ret->Init();
	NextRS++;
	return ret;
}


// Called with the other frame storage resets, after pending draws complete.
// Clearing a scene or starting a portal view must not reclaim these entries.
void R_ResetGhoul2Surfaces() { if(++jarSurfaceFrames%300==0 && jarSurfaceDrops) {
        if(ri.JAR_Log) ri.JAR_Log(1,"capacity","ghoul2_surfaces","dropped=%llu limit=%d",jarSurfaceDrops,MAX_RENDER_SURFACES);
        jarSurfaceDrops=0;
    }
    NextRS = 0; }
