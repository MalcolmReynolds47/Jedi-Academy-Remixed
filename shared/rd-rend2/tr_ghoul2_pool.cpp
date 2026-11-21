// From SomaZ/OpenJK's Rend2 Ghoul2 renderer. JA:R extraction and bounded
// allocation changes, GPL-2.0-only; see LICENSE.txt and docs/JAR-renderer-sources.md.
#include "tr_local.h"
static const int MAX_RENDERABLE_SURFACES = JAR_GHOUL2_RENDER_SURFACES;
static CRenderableSurface renderSurfHeap[MAX_RENDERABLE_SURFACES];
static int currentRenderSurfIndex = 0;
static unsigned long long jarSurfaceDrops=0;
static unsigned int jarSurfaceFrames=0;

CRenderableSurface *AllocGhoul2RenderableSurface()
{
	// Never overwrite surfaces still referenced by pending draws.
	if (currentRenderSurfIndex >= MAX_RENDERABLE_SURFACES) {
		if(++jarSurfaceDrops==1 && ri.JAR_Log) ri.JAR_Log(1,"capacity","ghoul2_surfaces_full","first drop in reporting interval");
		return nullptr;
	}

	CRenderableSurface *rs = &renderSurfHeap[currentRenderSurfIndex++];

	rs->Init();

	return rs;
}

void ResetGhoul2RenderableSurfaceHeap()
{
	if(++jarSurfaceFrames%300==0 && jarSurfaceDrops) {
        if(ri.JAR_Log) ri.JAR_Log(1,"capacity","ghoul2_surfaces","dropped=%llu limit=%d",jarSurfaceDrops,MAX_RENDERABLE_SURFACES);
        jarSurfaceDrops=0;
    }
    currentRenderSurfIndex = 0;
}

