/*
===========================================================================
Copyright (C) 2007-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_vbo.c
#include "tr_local.h"
size_t RB_AppendConstantsData(
	gpuFrame_t *frame, const void *data, size_t dataSize)
{
	const byte currentFrameScene = frame->currentScene;
	if (currentFrameScene >= MAX_SCENES)
		Com_Error(ERR_DROP, "JA:R Rend2: scene capacity exhausted");
	const size_t writeOffset = frame->uboWriteOffset[currentFrameScene];
	const size_t relativeOffset = writeOffset - frame->uboMapBase[currentFrameScene];

	const size_t alignment = glRefConfig.uniformBufferOffsetAlignment - 1;
	const size_t alignedBlockSize = (dataSize + alignment) & ~alignment;
	if (!frame->uboMemory[currentFrameScene] || writeOffset > frame->uboSize[currentFrameScene] ||
		alignedBlockSize < dataSize || alignedBlockSize > frame->uboSize[currentFrameScene] - writeOffset)
		Com_Error(ERR_DROP, "JA:R Rend2: GPU constant buffer capacity exhausted");
	memcpy((char *)frame->uboMemory[currentFrameScene] + relativeOffset, data, dataSize);

	frame->uboWriteOffset[currentFrameScene] += alignedBlockSize;
	assert(frame->uboWriteOffset[currentFrameScene] > 0);
	return writeOffset;
}

