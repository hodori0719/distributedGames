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
#ifndef __RTREE__H
#define __RTREE__H   

#include <list>
#include <util/debug.h>
#include <util/Benchmark.h>
#include <util/Rect.h>
#include <util/callback.h>         // not strictly needed, you can have non-curried callbacks if you wish! 

#define LO(r,d)  (r)->extent.min[(d)] 
#define HI(r,d)  (r)->extent.max[(d)]

template <class T, class U>
class RTree;               // forward declaration

#define INDENT(os, indent)  do { \
    for (int i = 0; i < indent; i++) os << " "; } while (0)

    template <class T, class U>
    class RTreeNode
{
    template<class T1, class U1> friend class RTree;

    typedef list<RTreeNode<T, U> *> RTNList;
    typedef typename RTNList::iterator RTNListIter;
public:
    static const unsigned int m = 2;
    static const unsigned int M = 10;
private:
    int          dim;
    int          level;
    RTNList      children;   
    RTreeNode   *parent;
    Rect<U>      extent; 
    T           *record;
public:
    RTreeNode (int dim, T* obj) : dim (dim), level (0), parent (NULL)
	, record (obj), extent (Rect<U> (dim)) 
	{	
	    if (record) 
		record->GetExtent (&extent);

	    ASSERT (extent.dim == dim);
	}

    ~RTreeNode () {
	children.clear ();
	parent = NULL;
	record = NULL;
    }

    // unlike a "textbook" R-Tree, our records are 
    // wrapped in a RTreeNode structure 
    const bool IsLeaf () const { 
	return children.size () == 0 || children.front ()->record != NULL; 
    }

    U GetArea () { return extent.GetArea (); }

    void SetParent (RTreeNode<T, U>* parent) { this->parent = parent; }
    T *GetRecord () { return record; }

    void Print (ostream& os, int indent = 0) {
	INDENT (os, indent);
	os << merc_va ("level=%d me=%p pa=%p", level, this, parent) << endl;

	if (record != NULL) {
	    INDENT (os, indent);
	    os << *record << endl;
	}

	if (children.size () == 0)
	    return;

	INDENT (os, indent);
	os << extent << endl;

	for (RTNListIter it = children.begin (); it != children.end (); ++it) {
	    (*it)->Print (os, indent + 4);
	}
    }


    void CheckInvariants ();
    int GetNumRecords () { 
	if (IsLeaf ())
	    return children.size ();

	int nrec = 0;
	for (RTNListIter it = children.begin (); it != children.end (); ++it) 
	    nrec += (*it)->GetNumRecords ();
	return nrec;
    }

    int GetLevel () { return level; }

    int GetLevelTraverse () {
	if (IsLeaf ())
	    return 0;
	else 
	    return children.front ()->GetLevelTraverse () + 1;
    }

private:
    void SetChildren (RTNList& nch);
    void AdjustExtent ();

    RTreeNode<T, U>* SplitNode ();
    void LinearPickSeeds (RTreeNode<T, U>* seeds[]);

    RTreeNode<T, U>* ChooseLeaf (RTreeNode<T, U>* nent, int level = 0);
    RTreeNode<T, U>* Insert (RTreeNode<T, U>* nent);
    RTreeNode<T, U>* FindLeaf (T *obj, const Rect<U>& extent);
    void EraseRecord (T *obj);
    void Condense (RTreeNode<T, U>* todel, RTNList *porphans);

    void GetOverlaps (const Rect<U>& extent, typename callback<void, T*>::ref cb); // list<T *> *plist);

    void AddChild (RTreeNode<T, U>* child) {
	if (child->record != NULL)
	    level = 0;
	else
	    level = child->level + 1;

	child->parent = this;
	children.push_back (child);
    }

    void Destroy () {
	for (RTNListIter it = children.begin (); it != children.end (); ++it) {	
	    if (! (*it))
		continue;
	    (*it)->Destroy ();
	    delete *it;
	}
    }
};

template<class T, class U>
ostream& operator<< (ostream& os, RTreeNode<T, U>& node)
{
    node.Print (os);
    return os;
}

////////////////////////////////////////////////////////////
// RTree
// ////////////////////////////////////////////////////////
template<class T, class U>
class RTree
{
    int dim;
    RTreeNode<T, U> *root;
public:
    RTree (int dim);
    ~RTree ();

    /**
     * Insert @obj into the RTree. class T must implement
     * a GetExtent method returning a Rect<U>. Does not 
     * clone the object; it's the user's responsibility
     * to manage memory.
     **/
    void Insert (T *obj);

    /**
     * Erases the object @obj from the RTree. Does not 
     * delete the object. See @Insert above.
     **/
    bool Erase (T *obj);

    /**
     * Gets all objects in the RTree which overlap with 
     * @obj. class V must implement a GetExtent method. 
     **/
    template<class V> void GetOverlaps (V *obj, typename callback<void, T*>::ref cb); // list<T*> *plist);

    // debug
    void CheckInvariants () { 
	root->CheckInvariants (); 
    }

    int GetNumRecords () { return root->GetNumRecords (); }
    RTreeNode<T, U>* GetRoot () { return root; }
private:
    void CondenseTree (RTreeNode<T, U>* leaf);
    void InsertNode (RTreeNode<T, U> *node, int level);
};

template<class T, class U>
ostream& operator<< (ostream& os, RTree<T, U>& tree)
{
    tree.GetRoot ()->Print (os);
    return os;
}

template<class T, class U>
RTree<T, U>::RTree (int dim) : dim (dim)
{
    root = new RTreeNode<T, U> (dim, NULL /* no associated record */);    
}

template<class T, class U>
RTree<T, U>::~RTree ()
{
    root->Destroy ();
    delete root;
}

// Usage:: tree->Insert(record);
template<class T, class U>
void RTree<T, U>::Insert (T *obj)
{
    DB(1) << "inserting object " << obj << endl;

    RTreeNode<T, U> *nent;

    nent = new RTreeNode<T, U> (dim, obj);
    InsertNode (nent, 0);
}

template<class T, class U>
void RTree<T, U>::InsertNode (RTreeNode<T, U>* nent, int level)
{
    RTreeNode<T, U> *leaf, *nroot, *oroot;
    leaf = root->ChooseLeaf (nent, level);

    ASSERT (leaf != NULL);
    nroot = leaf->Insert (nent);
    if (nroot) { 
	DB(1) << "root has been split; adding new root" << endl;
	oroot = root;
	root = new RTreeNode<T, U> (dim, NULL);

	root->AddChild (oroot);
	root->AddChild (nroot);

	root->AdjustExtent ();
    }
}

template<class T, class U>
struct level_cmp {
    bool operator () (const RTreeNode<T, U>* na, const RTreeNode<T, U>* nb) const {
	return na->GetLevel () < nb->GetLevel ();
    }
};

template<class T, class U>
bool RTree<T, U>::Erase (T *obj)
{
    DB(-1) << "deleting object " << obj << endl;

    Rect<U> extent (dim);    
    obj->GetExtent (&extent);

    RTreeNode<T, U>* leaf = root->FindLeaf (obj, extent);
    ASSERT (leaf != NULL);
    if (!leaf) 
	return false;

    leaf->EraseRecord (obj);

    // assimilate children of under-full nodes at other places
    CondenseTree (leaf);

    // shorten tree, if needed
    if (root->children.size () == 1 && !root->IsLeaf ()) {
	RTreeNode<T, U>* oroot = root;
	root = root->children.front ();
	root->parent = NULL;
	delete oroot;
    }
    return true;
}

template<class T, class U>
void RTree<T, U>::CondenseTree (RTreeNode<T, U> *leaf)
{
    typename RTreeNode<T, U>::RTNList orphan_parents;
    leaf->Condense (NULL, &orphan_parents);

    // deal with the orphans

    for (typename RTreeNode<T, U>::RTNListIter it = orphan_parents.begin (); it != orphan_parents.end (); ++it) {	
	DB(1) << "orphan parent: " << *it << endl;

	for (typename RTreeNode<T, U>::RTNListIter oit = (*it)->children.begin (); oit != (*it)->children.end (); ++oit) {
	    RTreeNode<T, U>* orph = *oit;
	    DB(1) << "re-inserting orphan: " << *oit << endl;

	    // ugh; our convention of wrapping records in RTreeNodes
	    // is mega-painful. putting a list<T *> inside a RTreeNode
	    // would be much more convenient. 
	    if (orph->level == 0 && orph->record != NULL) 
		InsertNode (orph, 0);
	    else
		InsertNode (*oit, (*oit)->level + 1);
	}

	delete *it;
    }
}

template<class T, class U>
template<class V> 
void RTree<T, U>::GetOverlaps (V *obj, typename callback<void, T*>::ref cb)
{
    Rect<U> extent (dim);
    obj->GetExtent (&extent);
    return root->GetOverlaps (extent, cb);
}

template<class T, class U>
void RTreeNode<T, U>::GetOverlaps (const Rect<U>& rect, typename callback<void, T*>::ref cb)
{
    if (IsLeaf ()) {
	for (RTNListIter it = children.begin (); it != children.end (); ++it) {
	    if ((*it)->extent.Overlaps (rect)) {
		cb ((*it)->record);
		// plist->push_back ((*it)->record);
	    }
	}
	return;
    }

    for (RTNListIter it = children.begin (); it != children.end (); ++it) {
	if ((*it)->extent.Overlaps (rect))
	    (*it)->GetOverlaps (rect, cb);
    }
}

template<class T, class U>
void RTreeNode<T, U>::EraseRecord (T *obj)
{
    for (RTNListIter it = children.begin (); it != children.end (); ++it) {
	if ((*it)->record && (*it)->record == obj) {
	    delete *it;
	    children.erase (it);
	    AdjustExtent ();
	    return;
	}
    }    
}

template<class T, class U>
void RTreeNode<T, U>::Condense (RTreeNode<T, U>* todel, RTNList *porphans)
{
    if (todel) {
	for (RTNListIter it = children.begin (); it != children.end (); ++it) {
	    if (*it == todel) {
		children.erase (it);
		break;
	    }
	}
    }
    if (children.size () >= m) 
	return;

    if (parent) {
	porphans->push_back (this); 
	parent->Condense (this, porphans);
    }
}

template<class T, class U>
RTreeNode<T, U>* RTreeNode<T, U>::FindLeaf (T *obj, const Rect<U>& extent)
{
    if (IsLeaf ()) {
	// our definition of a leaf is a little weird. 
	for (RTNListIter it = children.begin (); it != children.end (); ++it) {
	    if ((*it)->record && (*it)->record == obj)
		return this;
	}
	return NULL;
    }

    for (RTNListIter it = children.begin (); it != children.end (); ++it) {
	if ((*it)->extent.Overlaps (extent)) {
	    RTreeNode<T, U> *cand = (*it)->FindLeaf (obj, extent);
	    if (cand) 
		return cand;
	}
    }

    return NULL;
}

// Usage: root->ChooseLeaf (rect)
template<class T, class U>
RTreeNode<T, U>* RTreeNode<T, U>::ChooseLeaf (RTreeNode<T, U> *nent, int level)
{
    // if (level == 0), returns a leaf
    if (this->level == level) 
	return this;

    RTreeNode<T, U>* best = NULL;
    Rect<U> rc (dim);
    U mindiff = 0, diff = 0;

    // find childrect which needs to be least extended
    for (RTNListIter it = children.begin (); it != children.end (); ++it) {		
	rc = (*it)->extent;
	rc.Merge (nent->extent);
	diff = rc.GetArea () - (*it)->GetArea ();

#if 0
	/* no longer needed now, more confidence obtained :) */
	// XXX assumes type U is can be compared to zero
	ASSERTDO (diff >= -1.0e-4, { 
		WARN << "it.ext=" << (*it)->extent << " area=" << (*it)->GetArea() << endl;
		WARN << "nent.ext=" << nent->extent << endl;
		WARN << "merged.ext=" << rc << " area=" << rc.GetArea () << endl;
		WARN << "diff=" << diff << endl;
		}
		);  
#endif

	if (it == children.begin ()) {
	    mindiff = diff;
	    best = *it;
	}
	else {
	    if (diff < mindiff) { 
		mindiff = diff;
		best = *it;
	    }
	    // resolve ties
	    else if (diff == mindiff && (*it)->GetArea () < best->GetArea ()) {
		best = *it;
	    }
	}
    }

    ASSERT (best != NULL);
    return best->ChooseLeaf (nent, level);
}

// returns the new root node if insertion caused the root 
// node to split
template<class T, class U>
RTreeNode<T, U>* RTreeNode<T, U>::Insert (RTreeNode<T, U> *nent)
{
    RTreeNode<T, U>* newnode = NULL;
    if (nent) {
	AddChild (nent);

	if (children.size () > M) {
	    DB (1) << "splitting node " << this << endl;
	    newnode = SplitNode ();
	}
    }

    AdjustExtent ();
    if (newnode)
	newnode->AdjustExtent ();

    if (parent == NULL)
	return newnode;

    return parent->Insert (newnode);
}

template<class T, class U>
RTreeNode<T, U>* RTreeNode<T, U>::SplitNode ()
{
    RTreeNode<T, U> *seeds[2];
    RTNList          groups[2];
    Rect<U>         *cover[2];
    Rect<U>          rc(dim);
    U                diff[2];

    RTreeNode<T, U> *newnode = new RTreeNode<T, U> (dim, NULL);

    // pick seeds and make two groups
    LinearPickSeeds (seeds);

    DB(1) << "picked seeds as: " << endl;
    DB(1) << "\t" << seeds[0]->extent << endl;
    DB(1) << "\t" << seeds[1]->extent << endl;

    for (int i = 0; i < 2; i++) {
	groups[i].push_back (seeds[i]);
	cover[i] = new Rect<U> (dim);
	*cover[i] = seeds[i]->extent;
    }

    int toassn = children.size () - 2;
    for (RTNListIter it = children.begin (); it != children.end (); ++it) {
	if (*it == seeds[1] || *it == seeds[0]) 
	    continue;

	for (int i = 0; i < 2; i++) {
	    if (groups[i].size () + toassn <= m) {
		while (it != children.end ()) {
		    if (*it != seeds[0] && *it != seeds[1]) {
			groups[i].push_back (*it);
			toassn--;
		    }
		    ++it;
		}
		goto assndone;
	    }
	}

	for (int i = 0; i < 2; i++) {
	    // debug
	    DB(1) << "group [" << i << "] is: " << endl;
	    for (RTNListIter oit = groups[i].begin (); oit != groups[i].end (); ++oit)
		DB(1) << (*oit)->extent << endl;
	    DB(1) << "cover is: " << endl;
	    DB(1) << *cover[i] << endl;

	    // find enlargement area
	    rc = *cover[i];
	    rc.Merge ((*it)->extent);
	    diff[i] = rc.GetArea () - cover[i]->GetArea ();	    
	}

	int sel = -1;
	if (diff[0] < diff[1]) 
	    sel = 0;
	else if (diff[1] < diff[0]) 
	    sel = 1;
	else {
	    // resolve ties
	    if (cover[0]->GetArea () < cover[1]->GetArea ())
		sel = 0;
	    else if (cover[1]->GetArea () < cover[0]->GetArea ())
		sel = 1;
	    else {
		sel = rand () % 2;
	    }
	}

	ASSERT (sel == 0 || sel == 1);
	groups[sel].push_back (*it);
	toassn--;

	// update covering rect
	rc = *cover[sel];
	rc.Merge ((*it)->extent);
	*cover[sel] = rc;
    }
assndone:
    children.clear ();

    DB(1) << "finished splitting; newnode=" << newnode << endl;
    SetChildren (groups[0]);
    newnode->SetChildren (groups[1]);

    for (int i = 0; i < 2; i++)
	delete cover[i];

    ASSERT (newnode != this);
    return newnode;
}

template<class T, class U>
void RTreeNode<T, U>::CheckInvariants ()
{
    ASSERT(IsLeaf () || parent == NULL || 
	    (children.size () >= m && children.size () <= M));

    int lvl = 0;
    for (RTNListIter it = children.begin (); it != children.end (); ++it)
    {
	if (it == children.begin ()) 
	    lvl = (*it)->GetLevelTraverse ();
	else 
	    ASSERT ((*it)->GetLevelTraverse () == lvl);

	if ((*it)->record != NULL)
	    ASSERT (level == 0);
	else 
	    ASSERT (level == (*it)->level + 1);
    }
}

template<class T, class U>
void RTreeNode<T, U>::SetChildren (RTNList& nch)
{
    for (RTNListIter it = nch.begin (); it != nch.end (); ++it)
	AddChild (*it);
}

template<class T, class U>
void RTreeNode<T, U>::AdjustExtent ()
{
    DB(2) << "adjusting extent; current is" << endl << "\t" << extent << endl;;

    for (RTNListIter it = children.begin (); it != children.end (); ++it) {
	if (it == children.begin ())
	    extent = (*it)->extent;
	else 
	    extent.Merge ((*it)->extent);
    }
    DB(2) << "adjusted; new extent is" << endl << "\t" << extent << endl;;
}

template<class T, class U>
void RTreeNode<T, U>::LinearPickSeeds (RTreeNode<T, U> *seeds[]) 
{
    RTreeNode<T, U> *lowest_high = NULL, *highest_low = NULL;
    U best_sep = 0, best_span = 0;

    // find extreme rectangles
    for (int d = 0; d < dim; d++) {
	U wl = 0, wh = 0, span = 0, sep = 0;

	for (RTNListIter it = children.begin (); it != children.end (); ++it) {
	    // DB (-1) << "linpick: low=" << LO(*it, d) << " hi=" << HI(*it, d) << endl;

	    if (it == children.begin ()) {
		lowest_high = highest_low = *it;		
		wl = LO(*it, d);
		wh = HI(*it, d);
	    }
	    else {
		if (LO(*it, d) > LO(highest_low, d)) 
		    highest_low = *it;
		if (HI(*it, d) < HI(lowest_high, d)) 
		    lowest_high = *it;

		// maintain the span
		if (LO(*it, d) < wl) 
		    wl = LO(*it, d);
		if (HI(*it, d) > wh)
		    wh = HI(*it, d);
	    }
	}

	// ugh; complication. undocumented in Gutt84!
	// happens when there's a rectangle which is 
	// enclosed entirely by all other rectangles.
	// 
	// i dont have a good solution. will just choose 
	// a random rectangle in this case.
	if (lowest_high == highest_low) {
	    for (RTNListIter oit = children.begin (); oit != children.end (); ++oit) {
		if (*oit == lowest_high) 
		    continue;
		highest_low = *oit;
		break;
	    }
	}

	ASSERT (lowest_high != highest_low);

	span = wh - wl;
	sep  = LO(highest_low, d) - HI(lowest_high, d);

	ASSERT (span >= 0);
	ASSERT (sep <= span);

	DB(1) << "for dim=" << d << " sep=" << sep << " span=" << span << endl;
	DB(1) << "candidate rectangles are: " << endl;
	DB(1) << "\t" << lowest_high->extent << endl;
	DB(1) << "\t" << highest_low->extent << endl;

	// a1 * b2 > a2 * b1 <=> a1/b1 > a2/b2
	if (d == 0 || sep * best_span > best_sep * span) {
	    seeds[0] = lowest_high;
	    seeds[1] = highest_low;

	    best_span = span;
	    best_sep  = sep;
	}
    }
    return;
}

#endif /* __RTREE__H */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
