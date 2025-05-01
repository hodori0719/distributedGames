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
#ifndef __RECT__H
#define __RECT__H   

#include <util/debug.h>

// can templatized by the base type: float, int, MercuryID
//
// TODO: specialize by adding a dimension as a template parameter
// as well. this can avoid the 'new's and 'delete's. 
template<class T>
class Rect {
 public:
    int dim;
    T *min;
    T *max;

    Rect (int dim) : dim (dim) {
	min = new T[dim];
	max = new T[dim];

	/// XXX hope that a (-1, -1) ... rect is hugely uncommon
	for (int i = 0; i < dim; i++) 
	    min[i] = max[i] = -1; 
    }

    Rect (const Rect<T>& other) : dim (other.dim) {
	min = new T[dim];
	max = new T[dim];

	for (int i = 0; i < dim; i++) {
	    min[i] = other.min[i], max[i] = other.max[i];
	    ASSERT (min[i] <= max[i]);
	}
    }

    ~Rect () {
	delete[] min;
	delete[] max;
    }

    Rect<T>& operator= (const Rect<T>& other) {
	ASSERT (dim == other.dim);

	if (this == &other) 
	    return *this;
	for (int i = 0; i < dim; i++) {
	    min[i] = other.min[i], max[i] = other.max[i];
	    ASSERT (min[i] <= max[i]);
	}
	return *this;
    }

    void Merge (const Rect<T>& other) {
	ASSERT (dim == other.dim);

	for (int i = 0; i < dim; i++) {
	    // check for the uninitialized case.
	    if (min[i] == -1 && max[i] == -1)
		min[i] = other.min[i], max[i] = other.max[i];
	    else {
		if (other.min[i] < min[i])
		    min[i] = other.min[i];
		if (other.max[i] > max[i])
		    max[i] = other.max[i];
	    }
	}
    }

    bool Overlaps (const Rect<T>& other) {
	ASSERT (dim == other.dim);

	for (int i = 0; i < dim; i++) {
	    if (min[i] > other.max[i] || max[i] < other.min[i])
		return false;
	}
	return true;
    }

    T GetArea () const {
	T ret = 0;

	ASSERT (dim >= 0);
	for (int i = 0; i < dim; i++) 
	    if (i == 0)
		ret = (max[i] - min[i]);
	    else
		ret *= (max[i] - min[i]);
	return ret;
    }

    void GetExtent (Rect<T> *extent) { 
	*extent = *this;
    }
};

template<class T>
ostream& operator<< (ostream& os, Rect<T>& rect) {
    for (int i = 0; i < rect.dim; i++) { 
	if (i != 0) 
	    os << " x " ;
	os << "[" << rect.min[i] << ", " << rect.max[i] << "]";
    }
    return os;
}

#endif /* __RECT__H */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
