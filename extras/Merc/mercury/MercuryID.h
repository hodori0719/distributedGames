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

#ifndef __MERCURYID__H
#define __MERCURYID__H

// $Id: MercuryID.h 2507 2005-12-21 22:05:36Z jeffpang $

/* Copied from the SFS code. sfs1/crypt/bigint.h - Ashwin [02/12/2005] */

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

#include <mercury/common.h>
#include <string>
#include <gmp.h>

/* XXX FIXME: this information will ultimately be found out by 'configure' 
 * when we migrate to the GNU auto* files for distribution. meanwhile, 
 * i have just hard-coded it since this will work for all Redhat 8/9
 * machines we will be using. - Ashwin [02/13/2005] */

#define GMP_LIMB_SIZE 4
#define SIZEOF_LONG  4
#define SIZEOF_LONG_LONG 8

struct AttrInfo {
    int    index;
    string name;
};

extern AttrInfo *g_MercuryAttrRegistry;
extern int g_NumHubs;

inline const char *G_GetTypeName(int attrindex) { 
    if (g_NumHubs == 0)
	return "(undef)";
    if (g_MercuryAttrRegistry == NULL)
	return "(undef)";
    if (attrindex >= g_NumHubs || attrindex < 0)
	return "(oob)"; 
	
    return g_MercuryAttrRegistry[attrindex].name.c_str();
}

/// bigint goo

#undef ABS
#define ABS(a) ((a) < 0 ? -(a) : (a))

enum { mpz_bitsperlimb = sizeof (mp_limb_t) * 8 };

int mpz_getbit (const MP_INT *, size_t);
void mpz_square (MP_INT *, const MP_INT *);
size_t mpz_sizeinbase2 (const MP_INT *mp);
size_t mpz_rawsize (const MP_INT *);
void mpz_get_raw (char *buf, size_t size, const MP_INT *mp);
void mpz_set_raw (MP_INT *mp, const char *buf, size_t size);
void mpz_get_rawmag_le (char *buf, size_t size, const MP_INT *mp);
void mpz_get_rawmag_be (char *buf, size_t size, const MP_INT *mp);
void mpz_set_rawmag_le (MP_INT *mp, const char *buf, size_t size);
void mpz_set_rawmag_be (MP_INT *mp, const char *buf, size_t size);

#if SIZEOF_LONG >= 8
# define mpz_set_u64 mpz_set_ui
# define mpz_set_s64 mpz_set_si
# define mpz_get_u64 mpz_get_ui
# define mpz_get_s64 mpz_get_si
#else /* SIZEOF_LONG < 8 */
void mpz_set_u64 (MP_INT *mp, u_int64_t val);
void mpz_set_s64 (MP_INT *mp, int64_t val);
u_int64_t mpz_get_u64 (const MP_INT *mp);
int64_t mpz_get_s64 (const MP_INT *mp);
#endif /* SIZEOF_LONG < 8 */

void _mpz_fixsize (MP_INT *r);

#ifdef CHECK_BOUNDS
#define _mpz_assert(mp) assert (!(mp)->_mp_size || (mp)->_mp_d[ABS((mp)->_mp_size)-1])
#else /* !CHECK_BOUNDS */
#define _mpz_assert(mp) ((void) 0)
#endif

template<class A, class B = void, class C = void> class mpdelayed;

/**
 * This class should not be extended because it is difficult 
 * to do so in any meaningful way. Basically, any 'id' needs 
 * to behave like an integer (supporting comparison, addition, 
 * and division). 
 **/

class MercuryID : virtual public Serializable, public MP_INT {
 private:
 public:

    MercuryID () { mpz_init (this); }
    MercuryID (const MercuryID &b) { mpz_init_set (this, &b); }
    explicit MercuryID (const MP_INT *bp) { mpz_init_set (this, bp); }
    MercuryID (long si) { mpz_init_set_si (this, si); }
    MercuryID (u_long ui) { mpz_init_set_ui (this, ui); }
    MercuryID (int si) { mpz_init_set_si (this, si); }
    MercuryID (u_int ui) { mpz_init_set_ui (this, ui); }

    const static MercuryID ZERO;
    const static MercuryID ONE;

    explicit MercuryID (const char *s, int base = 0)
	{
	    // Jeff: this is a bit hacky, but wanted an easy way to say
	    // "max integer with X bits" (start the string with b)
	    if (s[0] == '%') {
		mpz_init_set_ui(this, 1);
		char *end = NULL;
		int pow = strtol(s+1, &end, base);
		ASSERT(*end == '\0');
		mpz_mul_2exp (this, this, pow);
		mpz_sub_ui(this, this, 1);
	    } else {
		int ret = mpz_init_set_str (this, s, base);
		if (ret != 0) {
		    WARN << "bad merc id string: " << s << " base=" << base << endl;
		    ASSERT(0);
		}
	    } 
	}
    template<class A, class B, class C>
	MercuryID (const mpdelayed<A, B, C> &d)
	{ mpz_init (this); d.getres (this); }

    virtual ~MercuryID () { mpz_clear (this); }

    MercuryID &operator= (const MercuryID &b) { 
	if (this == &b) return  *this; 
	mpz_set (this, &b); 
	return *this; 
    }
    MercuryID &operator= (const MP_INT *bp) { mpz_set (this, bp); return *this; }
    MercuryID &operator= (int si) { mpz_set_si (this, si); return *this; }
    MercuryID &operator= (long si) { mpz_set_si (this, si); return *this; }
    MercuryID &operator= (u_long ui) { mpz_set_ui (this, ui); return *this; }
    MercuryID &operator= (const char *s)
	{ mpz_set_str (this, s, 0); return *this; }

#if SIZEOF_LONG < 8
    MercuryID (int64_t si) { mpz_init (this); mpz_set_s64 (this, si); }
    MercuryID (u_int64_t ui) { mpz_init (this); mpz_set_u64 (this, ui); }
    MercuryID &operator= (int64_t si) { mpz_set_s64 (this, si); return *this; }
    MercuryID &operator= (u_int64_t ui) { mpz_set_u64 (this, ui); return *this; }
    int64_t gets64 () const { return mpz_get_s64 (this); }
    u_int64_t getu64 () const { return mpz_get_u64 (this); }
#else /* SIZEOF_LONG >= 8 */
    int64_t gets64 () const { return mpz_get_si (this); }
    u_int64_t getu64 () const { return mpz_get_ui (this); }
#endif /* SIZEOF_LONG >= 8 */
	
    void swap (MP_INT *a) {
	MP_INT t = *a;
	*a = *static_cast<const MP_INT *> (this);
	*static_cast<MP_INT *> (this) = t;
    }
    void swap (MercuryID &b) { swap (&b); }

    long getsi () const { return mpz_get_si (this); }
    u_long getui () const { return mpz_get_ui (this); }

#define ASSOPX(X, fn)				\
	MercuryID &operator X (const MercuryID &b)		\
	{ fn (this, this, &b); return *this; }	\
	MercuryID &operator X (const MP_INT *bp)		\
	{ fn (this, this, bp); return *this; }	\
	MercuryID &operator X (u_long ui)		\
	{ fn##_ui (this, this, ui); return *this; }
    ASSOPX (+=, mpz_add);
    ASSOPX (-=, mpz_sub);
    ASSOPX (*=, mpz_mul);
    ASSOPX (/=, mpz_tdiv_q);
    ASSOPX (%=, mpz_tdiv_r);
#undef ASSOPX
#define ASSOPX(X, fn)				\
	MercuryID &operator X (const MercuryID &b)		\
	{ fn (this, this, &b); return *this; }	\
	MercuryID &operator X (const MP_INT *bp)		\
	{ fn (this, this, bp); return *this; }
    ASSOPX (&=, mpz_and);
    ASSOPX (^=, mpz_xor);
    ASSOPX (|=, mpz_ior);
#undef ASSOPX

    MercuryID &operator<<= (u_long ui)
	{ mpz_mul_2exp (this, this, ui); return *this; }
    MercuryID &operator>>= (u_long ui)
	{ mpz_tdiv_q_2exp (this, this, ui); return *this; }

    const MercuryID &operator++ () { return *this += 1; }
    const MercuryID &operator-- () { return *this -= 1; }

    mpdelayed<const MP_INT *, u_long> operator++ (int);
    mpdelayed<const MP_INT *, u_long> operator-- (int);

    size_t nbits () const { return mpz_sizeinbase2 (this); }
    void (setbit) (u_long bitno, bool val) {
	if (val)
	    mpz_setbit (this, bitno);
	else
	    mpz_clrbit (this, bitno);
    }
    bool (getbit) (u_long bitno) const { return mpz_getbit (this, bitno); }
    void trunc (u_long size) { mpz_tdiv_r_2exp (this, this, size); }

    bool probab_prime (int reps = 25) const
	{ return mpz_probab_prime_p (this, reps); }
    u_long popcount () const
	{ return mpz_popcount (this); }

    /*
      bool operator== (const MercuryID& c) const {
      return mpz_cmp (this, &c) == 0;
      }
      bool operator!= (const MercuryID& c) const {
      return !(*this == c);
      }
      bool operator<= (const MercuryID& c) const {
      return mpz_cmp (this, &c) <= 0;
      }
      bool operator< (const MercuryID& c) const {
      return mpz_cmp (this, &c) < 0;
      }
      bool operator>= (const MercuryID &c) const {
      return mpz_cmp (this, &c) >= 0;
      }
      bool operator> (const MercuryID& c) const {
      return mpz_cmp (this, &c) > 0;
      }
    */
		
    // divides to bigint's to give a double - Ashwin [02/17/2005]

    double double_div (const MercuryID& c) const {
	mpq_t    num, den, res;
	double   ret;

	mpq_init (num); 
	mpq_init (den);
	mpq_init (res);

	mpq_set_z (num, this);
	mpq_set_z (den, &c);
	mpq_div (res, num, den);
	ret = mpq_get_d (res);
		
	mpq_clear (res);
	mpq_clear (den);
	mpq_clear (num);

	return ret;
    }
	
    void double_mul (double d);

#if 0
    // this causes severe loss of precision for obvious reasons!

    /** multiply this bigint by a double < 1.0f, and retain the integer part **/
    MercuryID& double_mul (double d) {
	if (d > 1.0f)
	    return *this;
		
	if (d == 0.0f) {
	    mpz_set_ui ((MP_INT *) this, 0);
	}
	double inv = 1.0f / d;
	mpz_tdiv_q_ui (this, this, (unsigned long int) inv);
	return *this;
    }
#endif

    MercuryID (Packet *pkt);
    void Serialize(Packet *pkt);
    uint32 GetLength();
    void Print(FILE *stream);
};

///////////////////////////////////////////////////////////////////////////////

// Jeff [7/20/2005]: HACK more junk pulled from sfs/crypt/bigint.h (cvs)
#define bigint MercuryID

#ifdef BIGINT_BOOLOP
#define MPDELAY_BOOLOP					\
public:							\
  operator bool() const { return bigint (*this); }	\
private:						\
  operator int() const;					\
public:
#else /* !BIGINT_BOOLOP */
#define MPDELAY_BOOLOP
#endif /* !BIGINT_BOOLOP */

template<> class mpdelayed<const MP_INT *> {
    typedef const MP_INT *A;
    typedef void (*fn_t) (MP_INT *, A);
    fn_t f;
    const A a;
 public:
    mpdelayed (fn_t ff, A aa) : f (ff), a (aa) {}
    void getres (MP_INT *r) const { f (r, a); }
    MPDELAY_BOOLOP
	};
template<class AA, class AB, class AC>
							   class mpdelayed<mpdelayed <AA, AB, AC> > {
    typedef mpdelayed <AA, AB, AC> A;
    typedef void (*fn_t) (MP_INT *, const MP_INT *);
    fn_t f;
    const A &a;
							   public:
    mpdelayed (fn_t ff, const A &aa) : f (ff), a (aa) {}
    void getres (MP_INT *r) const
	{ a.getres (r); f (r, r); }
    MPDELAY_BOOLOP
	};

template<class B> class mpdelayed<const MP_INT *, B> {
    typedef const MP_INT *A;
    typedef void (*fn_t) (MP_INT *, A, B);
    fn_t f;
    const A a;
    const B b;
 public:
    mpdelayed (fn_t ff, A aa, B bb) : f (ff), a (aa), b (bb) {}
    void getres (MP_INT *r) const { f (r, a, b); }
    MPDELAY_BOOLOP
	};
template<class BA, class BB, class BC>
							   class mpdelayed<const MP_INT *, mpdelayed<BA, BB, BC> > {
    typedef const MP_INT *A;
    typedef mpdelayed <BA, BB, BC> B;
    typedef void (*fn_t) (MP_INT *, const MP_INT *, const MP_INT *);
    fn_t f;
    const A a;
    const B &b;
							   public:
    mpdelayed (fn_t ff, A aa, const B &bb) : f (ff), a (aa), b (bb) {}
    void getres (MP_INT *r) const {
	if (r == a) {
	    bigint t = b;
	    f (r, a, &t);
	}
	else {
	    b.getres (r);
	    f (r, a, r);
	}
    }
    MPDELAY_BOOLOP
	};

template<class AA, class AB, class AC>
							   class mpdelayed<mpdelayed<AA, AB, AC>, const MP_INT *> {
    typedef mpdelayed <AA, AB, AC> A;
    typedef const MP_INT *B;
    typedef void (*fn_t) (MP_INT *, const MP_INT *, B);
    fn_t f;
    const A &a;
    const B b;
							   public:
    mpdelayed (fn_t ff, const A &aa, B bb) : f (ff), a (aa), b (bb) {}
    void getres (MP_INT *r) const {
	if (r == b) {
	    bigint t = a;
	    f (r, &t, b);
	}
	else {
	    a.getres (r);
	    f (r, r, b);
	}
    }
    MPDELAY_BOOLOP
	};
template<class AA, class AB, class AC>
							   class mpdelayed<mpdelayed<AA, AB, AC>, u_long> {
    typedef mpdelayed <AA, AB, AC> A;
    typedef const u_long B;
    typedef void (*fn_t) (MP_INT *, const MP_INT *, B);
    fn_t f;
    const A &a;
    const B b;
							   public:
    mpdelayed (fn_t ff, const A &aa, B bb) : f (ff), a (aa), b (bb) {}
    void getres (MP_INT *r) const
	{ a.getres (r); f (r, r, b); }
    MPDELAY_BOOLOP
	};

template<class BA, class BB, class BC>
							   class mpdelayed<bigint, mpdelayed<BA, BB, BC> > {
    typedef bigint A;
    typedef mpdelayed <BA, BB, BC> B;
    typedef void (*fn_t) (MP_INT *, const MP_INT *, const MP_INT *);
    fn_t f;
    const A a;
    const B &b;
							   public:
    mpdelayed (fn_t ff, const A &aa, const B &bb) : f (ff), a (aa), b (bb) {}
    template<class TA, class TB, class TC>
	mpdelayed (fn_t ff, const mpdelayed<TA, TB, TC> &aa, const B &bb)
	: f (ff), a (aa), b (bb) {}
    void getres (MP_INT *r) const
	{ b.getres (r); f (r, a, r); }
    MPDELAY_BOOLOP
	};

template<class A, class B> class mpdelayed<A, B> {
    typedef void (*fn_t) (MP_INT *, A, B);
    fn_t f;
    const A a;
    const B b;
 public:
    mpdelayed (fn_t ff, A aa, B bb) : f (ff), a (aa), b (bb) {}
    void getres (MP_INT *r) const { f (r, a, b); }
    MPDELAY_BOOLOP
	};

template<class A, class B, class C> class mpdelayed {
    typedef void (*fn_t) (MP_INT *, A, B, C);
    fn_t f;
    const A a;
    const B b;
    const C c;
 public:
    mpdelayed (fn_t ff, A aa, B bb, C cc) : f (ff), a (aa), b (bb), c (cc) {}
    void getres (MP_INT *r) const { f (r, a, b, c); }
    MPDELAY_BOOLOP
	};

#undef MPDELAY_BOOLOP

#define MPMPOP(X, F)							\
inline mpdelayed<const MP_INT *, const MP_INT *>			\
X (const bigint &a, const bigint &b)					\
{									\
  return mpdelayed<const MP_INT *, const MP_INT *> (F, &a, &b);		\
}									\
inline mpdelayed<const MP_INT *, const MP_INT *>			\
X (const bigint &a, const MP_INT *b)					\
{									\
  return mpdelayed<const MP_INT *, const MP_INT *> (F, &a, b);		\
}									\
inline mpdelayed<const MP_INT *, const MP_INT *>			\
X (const MP_INT *a, const bigint &b)					\
{									\
  return mpdelayed<const MP_INT *, const MP_INT *> (F, a, &b);		\
}									\
template<class A, class B, class C>					\
inline mpdelayed<const MP_INT *, mpdelayed<A, B, C> >			\
X (const bigint &a, const mpdelayed<A, B, C> &b)			\
{									\
  return mpdelayed<const MP_INT *, mpdelayed<A, B, C> > (F, &a, b);	\
}									\
template<class A, class B, class C>					\
inline mpdelayed<mpdelayed<A, B, C>, const MP_INT *>			\
X (const mpdelayed<A, B, C> &a, const bigint &b)			\
{									\
  return mpdelayed<mpdelayed<A, B, C>, const MP_INT *> (F, a, &b);	\
}									\
template<class AA, class AB, class AC, class BA, class BB, class BC>	\
inline mpdelayed<bigint, mpdelayed<BA, BB, BC> >			\
X (const mpdelayed<AA, AB, AC> &a, const mpdelayed<BA, BB, BC> &b)	\
{									\
  return mpdelayed<bigint, mpdelayed<BA, BB, BC> > (F, a, b);		\
}
#define MPUIOP(X, F)						\
inline mpdelayed<const MP_INT *, u_long>			\
X (const bigint &a, u_long b)					\
{								\
  return mpdelayed<const MP_INT *, u_long> (F, &a, b);		\
}								\
template<class A, class B, class C>				\
inline mpdelayed<mpdelayed<A, B, C>, u_long>			\
X (const mpdelayed<A, B, C> &a, u_long b)			\
{								\
  return mpdelayed<mpdelayed<A, B, C>, u_long> (F, a, b);	\
}
#define MPUICOP(X, F)						\
MPUIOP (X, F)							\
inline mpdelayed<const MP_INT *, u_long>			\
X (u_long b, const bigint &a)					\
{								\
  return mpdelayed<const MP_INT *, u_long> (F, &a, b);		\
}								\
template<class A, class B, class C>				\
inline mpdelayed<mpdelayed<A, B, C>, u_long>			\
X (u_long b, const mpdelayed<A, B, C> &a)			\
{								\
  return mpdelayed<mpdelayed<A, B, C>, u_long> (F, a, b);	\
}
#define UNOP(X, F)					\
inline mpdelayed<const MP_INT *>			\
X (const bigint &a)					\
{							\
  return mpdelayed<const MP_INT *> (F, &a);		\
}							\
template<class A, class B, class C>			\
inline mpdelayed<mpdelayed <A, B, C> >			\
X (const mpdelayed<A, B, C> &a)				\
{							\
  return mpdelayed<mpdelayed <A, B, C> > (F, a);	\
}

#define BINOP(X, F) MPMPOP (X, F) MPUIOP (X, F##_ui)
#define CBINOP(X, F) MPMPOP (X, F) MPUICOP (X, F##_ui)

#define CMPOPX(X)						\
inline bool							\
operator X (const bigint &a, const bigint &b)			\
{								\
  return mpz_cmp (&a, &b) X 0;					\
}								\
inline bool							\
operator X (const bigint &a, const MP_INT *bp)			\
{								\
  return mpz_cmp (&a, bp) X 0;					\
}								\
inline bool							\
operator X (const bigint &a, u_long ui)				\
{								\
  return mpz_cmp_ui (&a, ui) X 0;				\
}								\
inline bool							\
operator X (const bigint &a, long si)				\
{								\
  return mpz_cmp_si (&a, si) X 0;				\
}								\
inline bool							\
operator X (const bigint &a, u_int ui)				\
{								\
  return mpz_cmp_ui (&a, ui) X 0;				\
}								\
inline bool							\
operator X (const bigint &a, int si)				\
{								\
  return mpz_cmp_si (&a, si) X 0;				\
}								\
inline bool							\
operator X (const MP_INT *bp, const bigint &b)			\
{								\
  return mpz_cmp (bp, &b) X 0;					\
}								\
template<class AA, class AB, class AC, class B> inline bool	\
operator X (const mpdelayed<AA, AB, AC> &_a, const B &b)	\
{								\
  bigint a (_a);						\
  return a X b;							\
}								\
template<class AA, class AB, class AC> inline bool		\
operator X (const bigint &a, const mpdelayed<AA, AB, AC> &_b)	\
{								\
  bigint b (_b);						\
  return a X b;							\
}

CBINOP (operator*, mpz_mul)
CBINOP (operator+, mpz_add)
BINOP (operator-, mpz_sub)

/* Need to get rid of return values for GMP version 3 */
MPMPOP (operator/, mpz_tdiv_q)
inline void
mpz_tdiv_q_ui_void (MP_INT *r, const MP_INT *a, u_long b)
{
    mpz_tdiv_q_ui (r, a, b);
}
MPUIOP (operator/, mpz_tdiv_q_ui_void)

MPMPOP (operator%, mpz_tdiv_r)
inline void
mpz_tdiv_r_ui_void (MP_INT *r, const MP_INT *a, u_long b)
{
    mpz_tdiv_r_ui (r, a, b);
}
/* Note: mod(bigint, u_long) is more efficient (but has result always >= 0) */
MPUIOP (operator%, mpz_tdiv_r_ui_void)

MPMPOP (mod, mpz_mod)

MPMPOP (operator&, mpz_and)
MPMPOP (operator^, mpz_xor)
MPMPOP (operator|, mpz_ior)
MPMPOP (gcd, mpz_gcd)

MPUIOP (operator<<, mpz_mul_2exp)
MPUIOP (operator>>, mpz_tdiv_q_2exp)

UNOP (operator-, mpz_neg)
UNOP (operator~, mpz_com)
UNOP (abs, mpz_abs)
UNOP (sqrt, mpz_sqrt)

CMPOPX (<)
CMPOPX (>)
CMPOPX (<=)
CMPOPX (>=)
CMPOPX (==)
CMPOPX (!=)

#undef MPMPOP
#undef MPUIOP
#undef MPUICOP
#undef BINOP
#undef CBINOP
#undef UNOP
#undef CMOPOPX

inline int
sgn (const bigint &a)
{
    return mpz_sgn (&a);
}
inline bool
operator! (const bigint &a)
{
    return !sgn (a);
}
inline const bigint &
operator+ (const bigint &a)
{
    return a;
}

inline void
_invert0 (MP_INT *r, const MP_INT *a, const MP_INT *b)
{
    bigint gcd;
    mpz_gcdext (&gcd, r, NULL, a, b);
    if (gcd._mp_size != 1 || gcd._mp_d[0] != 1)
	r->_mp_size = 0;
    else if (r->_mp_size < 0)
	mpz_add (r, r, b);
}

inline mpdelayed<const MP_INT *, const MP_INT *>
							 invert (const bigint &a, const bigint &mod)
{
    return mpdelayed<const MP_INT *, const MP_INT *> (&_invert0, &a, &mod);
}

inline mpdelayed<const MP_INT *, u_long>
							 pow (const bigint &a, u_long ui)
{
    return mpdelayed<const MP_INT *, u_long> (mpz_pow_ui, &a, ui);
}

inline mpdelayed<const MP_INT *, const MP_INT *, const MP_INT *>
							 powm (const bigint &base, const bigint &exp, const bigint &mod)
{
    return mpdelayed<const MP_INT *, const MP_INT *, const MP_INT *>
	(mpz_powm, &base, &exp, &mod);
}
inline mpdelayed<const MP_INT *, const MP_INT *, const MP_INT *>
							 powm (const bigint &base, const bigint &exp, const MP_INT *mod)
{
    return mpdelayed<const MP_INT *, const MP_INT *, const MP_INT *>
	(mpz_powm, &base, &exp, mod);
}

inline mpdelayed<const MP_INT *, u_long, const MP_INT *>
							 powm (const bigint &base, u_long ui, const bigint &mod)
{
    return mpdelayed<const MP_INT *, u_long, const MP_INT *>
	(mpz_powm_ui, &base, ui, &mod);
}
inline mpdelayed<const MP_INT *, u_long, const MP_INT *>
							 powm (const bigint &base, u_long ui, const MP_INT *mod)
{
    return mpdelayed<const MP_INT *, u_long, const MP_INT *>
	(mpz_powm_ui, &base, ui, mod);
}

inline mpdelayed<const MP_INT *, u_long>
							 bigint::operator++ (int)
{
    return mpdelayed<const MP_INT *, u_long> (mpz_sub_ui, &++*this, 1);
}
inline mpdelayed<const MP_INT *, u_long>
							 bigint::operator-- (int)
{
    return mpdelayed<const MP_INT *, u_long> (mpz_add_ui, &--*this, 1);
}

inline u_long
mod (const bigint &a, u_long b)
{
    static bigint junk;
    return mpz_mod_ui (&junk, &a, b);
}

inline u_long
gcd (const bigint &a, u_long b)
{
    return mpz_gcd_ui (NULL, &a, b);
}
inline u_long
gcd (u_long b, const bigint &a)
{
    return mpz_gcd_ui (NULL, &a, b);
}

inline u_long
hamdist (const bigint &a, const bigint &b)
{
    return mpz_hamdist (&a, &b);
}

inline int
jacobi (const bigint &a, const bigint &b)
{
    return mpz_jacobi (&a, &b);
}

inline int
legendre (const bigint &a, const bigint &b)
{
    return mpz_legendre (&a, &b);
}

#undef bigint

///////////////////////////////////////////////////////////////////////////////

inline void
swap (MercuryID &a, MercuryID &b)
{
    a.swap (b);
}

ostream& operator<< (ostream& out, const MercuryID *id);
ostream& operator<< (ostream& out, const MercuryID &id);

typedef MercuryID Value;
bool IsBetween (const Value& test, const Value& left, const Value& right);
bool IsBetweenInclusive (const Value& test, const Value& left, const Value& right);
bool IsBetweenLeftInclusive (const Value& test, const Value& left, const Value& right);
bool IsBetweenRightInclusive (const Value& test, const Value& left, const Value& right);

extern Value VALUE_NONE;

#endif // __MERCURYID__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
