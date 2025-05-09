////////////////////////////////////////////////////////////////////////////////
// Mercury and Colyseus Software Distribution 
// 
// Copyright (C) 2004-2005 Ashwin Bharambe (ashu@cs.cmu.edu)
//               2004-2005 Jeffrey Pang    (jeffpang@cs.cmu.edu)
//                    2004 Mukesh Agrawal  (mukesh@cs.cmu.edu)
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2, or (at
// your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
////////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------------
//
//   Torque Network Library
//   Copyright (C) 2004 GarageGames.com, Inc.
//   For more information see http://www.opentnl.org
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   For use in products that are not compatible with the terms of the GNU 
//   General Public License, alternative licensing options are available 
//   from GarageGames.com.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifndef _DATACHUNKER_H_
#define _DATACHUNKER_H_

#include <new>
#include <util/types.h>
#include <util/debug.h>

//----------------------------------------------------------------------------

/// Constructs an object that already has memory allocated for it.
template <class T>
inline T* constructInPlace(T* p)
{
    return new(p) T;
}

/// Copy constructs an object that already has memory allocated for it.
template <class T>
inline T* constructInPlace(T* p, const T* copy)
{
    return new(p) T(*copy);
}

/// Destructs an object without freeing the memory associated with it.
template <class T>
inline void destructInPlace(T* p)
{
    p->~T();
}

//----------------------------------------------------------------------------
/// Implements a chunked data allocater.
///
/// Calling new/malloc all the time is a time consuming operation. Therefore,
/// we provide the DataChunker, which allocates memory in blocks of
/// chunkSize (by default 16k, see ChunkSize, though it can be set in
/// the constructor), then doles it out as requested, in chunks of up to
/// chunkSize in size.
///
/// It will assert if you try to get more than ChunkSize bytes at a time,
/// and it deals with the logic of allocating new blocks and giving out
/// word-aligned chunks.
///
/// Note that new/free/realloc WILL NOT WORK on memory gotten from the
/// DataChunker. This also only grows (you can call freeBlocks to deallocate
/// and reset things).
class DataChunker
{
 public:
    enum {
	ChunkSize = 16376 ///< Default size of each DataBlock page in the DataChunker
    };

 private:
    /// DataBlock representation for each page of the DataChunker
    struct DataBlock
    {
	DataBlock *next;        ///< linked list pointer to the next DataBlock for this chunker
	byte *data;             ///< allocated pointer for the base of this page
	sint32 curIndex;        ///< current allocation point within this DataBlock
	DataBlock(sint32 size);
	~DataBlock();
    };
    DataBlock *curBlock;       ///< current page we're allocating data from.  If the
    ///< data size request is greater than the memory space currently
    ///< available in the current page, a new page will be allocated.
    sint32 chunkSize;          ///< The size allocated for each page in the DataChunker
 public:
    void *alloc(sint32 size);  ///< allocate a pointer to memory of size bytes from the DataChunker
    void freeBlocks();         ///< free all pages currently allocated in the DataChunker

    DataChunker(sint32 size=ChunkSize); ///< Construct a DataChunker with a page size of size bytes.
    ~DataChunker();

    /// Swaps the memory allocated in one data chunker for another.  This can be used to implement
    /// packing of memory stored in a DataChunker.
    void swap(DataChunker &d)
	{
	    DataBlock *temp = d.curBlock;
	    d.curBlock = curBlock;
	    curBlock = temp;
	}
};

//----------------------------------------------------------------------------

/// Templatized data chunker class with proper construction and destruction of its elements.
///
/// DataChunker just allocates space. This subclass actually constructs/destructs the
/// elements. This class is appropriate for more complex classes.
template<class T>
class ClassChunker: private DataChunker
{
    sint32 numAllocated; ///< number of elements currently allocated through this ClassChunker
    sint32 elementSize;  ///< the size of each element, or the size of a pointer, whichever is greater
    T *freeListHead;     ///< a pointer to a linked list of freed elements for reuse
 public:
    ClassChunker(sint32 size = DataChunker::ChunkSize) : DataChunker(size)
	{
	    numAllocated = 0;
	    elementSize = MAX(uint32(sizeof(T)), uint32(sizeof(T *)));
	    freeListHead = NULL;
	}
    /// Allocates and properly constructs in place a new element.
    T *alloc()
	{
	    //#ifdef DEBUG
#if 0
	    // valgrind doesn't know how to detect memory problems with chunker
	    return new T();
#else
	    numAllocated++;
	    if(freeListHead == NULL)
		return constructInPlace(reinterpret_cast<T*>(DataChunker::alloc(elementSize)));
	    T* ret = freeListHead;
	    freeListHead = *(reinterpret_cast<T**>(freeListHead));
	    return constructInPlace(ret);
#endif
	}

    /// Properly destructs and frees an element allocated with the alloc method.
    void free(T* elem)
	{
	    // #ifdef DEBUG
#if 0
	    delete elem;
#else
	    destructInPlace(elem);
	    numAllocated--;
	    *(reinterpret_cast<T**>(elem)) = freeListHead;
	    freeListHead = elem;
#endif
	}
};

#endif
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
