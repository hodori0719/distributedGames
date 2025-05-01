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
#ifndef __AUTOSERIALIZER__H
#define __AUTOSERIALIZER__H

// $Id: AutoSerializer.h 2382 2005-11-03 22:54:59Z ashu $

#include <mercury/common.h>
#include <mercury/Packet.h>

/// Limitations: 
//
// registration of types should be done in the _same_ order
// in the sending (serializing) side as well as the
// receiving (deserializing) side. This will typically hold
// true for a P2P app which runs the _same_ code on each
// machine... if hash functions and typenames are used
// (weird macro tricks), then this can be made a function of
// types themselves.
//
// there is a macro-call still left (DECLARE_TYPE) which, if 
// eliminated, would be nice. The reason is that right now, it
// will be very difficult to hunt down a bug in the type 
// assignment code if the user accidentally forgets to add a 
// DECLARE_TYPE macro in his class! Having the compiler do it 
// automatically (or at least enforce it) would be very useful.

#define MAX_DER_TYPES 256         // max derived classes per type

typedef Serializable * (*ConstructorType)(Packet *pkt);

template<class T>
Serializable *ConstructType(Packet *pkt) {
    return new T(pkt);
}

struct TypeInfo {
    ConstructorType constructor;
    const char *description;
};

// this class stores information about each base class 'T' -
// in particular, it stores a mapping from subtype index to 
// the particular constructor. Each such type 'T' must
// inherit from 'Serializable'...

template<class T>
class BaseclassInfo {
protected:
    byte m_Type;
    BaseclassInfo(byte t) : m_Type(t) {}
public: 
    byte GetType() const { return m_Type; }

    static byte type_pool;
    static TypeInfo typeinfo[MAX_DER_TYPES];	
    static int  num_dtypes() { return (int) type_pool; }
};

template<class T>
byte BaseclassInfo<T>::type_pool = 0;

template<class T>
TypeInfo BaseclassInfo<T>::typeinfo[MAX_DER_TYPES];

// Decide which constructor to call.

template<class T>
T *CreateObject(Packet *pkt)
{
    byte id_byte = pkt->PeekByte();
    ASSERT(id_byte < BaseclassInfo<T>::num_dtypes());

    return dynamic_cast<T *>((BaseclassInfo<T>::typeinfo[id_byte].constructor)(pkt));
}

// this class stores information about the "type"-index
// assigned to each extended class ET for the base class T.
// RegisterType() will set this information. the non-static 
// BaseclassInfo() superclass will store this type in each
// object...

template<class T, class ET>
class PSerializable : public BaseclassInfo<T>
{
public:
    PSerializable () : BaseclassInfo<T>(PSerializable<T, ET>::type) { }
    static byte type;

    // Hmm.. cool. the following looks like a way to give
    // all derived classes a constructor of a specific
    // form! However, I will curb my enthusiasm for the
    // moment and not actually do it... - Ashwin [01/29/2005]
    //  
    // ET(Packet *pkt); 
};

template<class T, class ET>
byte PSerializable<T, ET>::type;

// Register an extended type ET into the typeinfo for base
// class T. Register its constructor... The 'type' returned 
// can be stored by the app for demultiplexing. For example, 
//     MSG_ET = RegisterType<T, ET>

template<class T, class ET>
int RegisterType(const char *description) 
{
    byte type;
    type = PSerializable<T, ET>::type = BaseclassInfo<T>::type_pool++;
    BaseclassInfo<T>::typeinfo[type].constructor = ConstructType<ET>;
    BaseclassInfo<T>::typeinfo[type].description = strdup (description);
    return type;
}

#define REGISTER_TYPE(T, ET)  RegisterType<T, ET>("ID_"#ET)

// Extended classes need to declare this macro 
// somewhere in their class definitions...

// the return type for Clone() uses Covariant-types standard 
// (the right virtual function _should_ be called)

#define DECLARE_TYPE(T, ET)     \
public:                         \
    PSerializable<T, ET>  info;    \
    virtual byte GetType() const { return info.GetType(); } \
    virtual ET* Clone () const { return new ET (*this); }   

    // use the following macro if you want to 
    // declare an abstract base type (which 
    // can't be instantiated.)

#define DECLARE_BASE_TYPE(T)     \
    public:                          \
    PSerializable<T, T>  info;    \
    virtual byte GetType() const { return info.GetType(); } \
    virtual T* Clone () const = 0;

#if 0

// THIS IS OLD CODE WHICH USES MACRO TRICKS

#include <hash_map.h>
#define MAX_BASE_TYPES 10
extern byte id_pool;

typedef Serializable * (*ConstructorType)(Packet *pkt);
typedef hash_map<byte, ConstructorType, hash<byte> > CtrMap;
typedef CtrMap::iterator CtrMapIter;

extern CtrMap s_ConstructorMaps[MAX_BASE_TYPES];

template<class T>
Serializable *ConstructType(Packet *pkt)
{
    return new T(pkt);
}

template<class T>
byte RegisterType(CtrMap &map)
{
    id_pool ++ ;
    map[id_pool] = ConstructType<T>;
    return id_pool;
}

template<class T>
byte RegisterType(int basetype)
{
    ASSERT(basetype >= 0);
    ASSERT(basetype < MAX_BASE_TYPES);
    return RegisterType<T>(s_ConstructorMaps[basetype]);
}

template<class T>
T *CreateObject(CtrMap &map, Packet *pkt)
{
    byte id = pkt->ReadByte();
    CtrMapIter it = map.find(id);
    ASSERT(it != map.end());

    return dynamic_cast<T *>((it->second)(pkt));
}

template<class T>
T *CreateObject(int basetype, Packet *pkt)
{
    ASSERT(basetype >= 0);
    ASSERT(basetype < MAX_BASE_TYPES);
    return CreateObject<T>(s_ConstructorMaps[basetype], pkt);
}

void Serialize(Serializable *obj, Packet *pkt)
{
    obj->WriteType(pkt);
    obj->Serialize(pkt);
}

extern int type_index_A;
int RegisterBaseType(char *nametype);

#define CREATE_OBJECT(Type, BaseType, pkt)  do {  \
    CreateObject<Type>(type_index_##BaseType, pkt); } while (0)

#define REGISTER_ID_TYPE(Id, Type, BaseType)             \
    type_index_##BaseType = RegisterBaseType(#BaseType);  \
    byte Id = RegisterType<Type>(__);    \
    class Type;                          \
    void Type::WriteType(Packet *pkt) { pkt->WriteByte(Id); } 

#endif

#endif /* __AUTOSERIALIZER__H */
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
