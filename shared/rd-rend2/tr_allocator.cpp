#include <cstdlib>
#include "tr_allocator.h"
#include "tr_local.h"

Allocator::Allocator( void *memory, size_t memorySize, size_t alignment )
	: alignment(alignment)
	, ownMemory(false)
	, unalignedBase(memory)
	, alignedBase(PADP(unalignedBase, alignment))
	, mark(alignedBase)
	, end((char *)unalignedBase + memorySize)
{
	assert(unalignedBase);
	assert(memorySize);
	assert(alignment);
}

Allocator::Allocator( size_t memorySize, size_t alignment )
	: alignment(alignment)
	, ownMemory(true)
#if defined(GLSL_BUILDTOOL)
	, unalignedBase(malloc(memorySize))
#else
	, unalignedBase(R_Malloc(memorySize, TAG_SHADERTEXT))
#endif
	, alignedBase(PADP(unalignedBase, alignment))
	, mark(alignedBase)
	, end((char *)unalignedBase + memorySize)
{
	assert(unalignedBase);
	assert(memorySize);
	assert(alignment);
}

Allocator::~Allocator()
{
	if ( ownMemory )
	{
#if defined(GLSL_BUILDTOOL)
		free(unalignedBase);
#else
		Z_Free(unalignedBase);
#endif
	}
}

void *Allocator::Base() const
{
	return alignedBase;
}

size_t Allocator::GetSize() const
{
	return (size_t)((char *)end - (char *)alignedBase);
}

void *Allocator::Alloc( size_t allocSize )
{
	const size_t alignedSize = PAD(allocSize, alignment);
	if (alignedSize < allocSize || (size_t)((char *)end - (char *)mark) < alignedSize)
	{
		#if !defined(GLSL_BUILDTOOL)
		Com_Error(ERR_DROP, "JA:R Rend2: frame allocation capacity exhausted");
#else
		assert(!"Allocator is out of memory");
#endif
		return nullptr;
	}

	void *result = mark;
	
	mark = (char *)mark + alignedSize;

	return result;
}

void *Allocator::Mark() const
{
	return mark;
}

void Allocator::Reset()
{
	mark = alignedBase;
}

void Allocator::ResetTo( void *m )
{
	mark = m;
}
