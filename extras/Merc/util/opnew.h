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
// -*-c++-*-
/* $Id: opnew.h 2382 2005-11-03 22:54:59Z ashu $ */

/*
 *
 * Copyright (C) 1998 David Mazieres (dm@uun.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#ifndef _NEW_H_INCLUDED_
#define _NEW_H_INCLUDED_ 1

#include <new>

using std::nothrow;

#ifdef DMALLOC
using std::nothrow_t;
static class dmalloc_init {
  static bool initialized;
  static void init ();
public:
  bool ok () { return initialized; }
  dmalloc_init () { if (!initialized) init (); }
} __dmalloc_init_obj;
struct dmalloc_t {};
extern struct dmalloc_t dmalloc;
void *operator new (size_t, dmalloc_t, const char *, int);
void *operator new[] (size_t, dmalloc_t, const char *, int);
void *operator new (size_t, nothrow_t, const char *, int) throw ();
void *operator new[] (size_t, nothrow_t, const char *, int) throw ();
#define ntNew new (nothrow, __FILE__, __LINE__)
#define New new (dmalloc, __FILE__, __LINE__)
#define opnew(size) operator new (size, dmalloc, __FILE__, __LINE__)
#if __GNUC__ >= 2
#define DSPRINTF_DEBUG 1
#endif /* GCC2 */
#else /* !DMALLOC */
#define ntNew new (nothrow)
#define New new
#define opnew(size) operator new(size)
#endif /* !DMALLOC */

#define vNew (void) New

template<class T> inline T
destroy_return (T &t)
{
  T ret = t;
  t.~T ();
  return ret;
}

// XXX - work around egcs 1.1.2 bug:
template<class T> inline T *
destroy_return (T *&tp)
{
  return tp;
}

#endif /* !_NEW_H_INCLUDED_ */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
