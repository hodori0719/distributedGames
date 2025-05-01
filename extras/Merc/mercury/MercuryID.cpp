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

// $Id: MercuryID.cpp 2507 2005-12-21 22:05:36Z jeffpang $

/* Most of the stuff in this file is from sfs/crypt and sfs/async 
 * in the sfs toolkit. http://www.fs.net/ */

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
#include <mercury/MercuryID.h>
#include <mercury/Packet.h>
#include <gmp.h>

const MercuryID MercuryID::ZERO = MercuryID(0);
const MercuryID MercuryID::ONE  = MercuryID(1);

Value VALUE_NONE = Value ("-1", 10);

#define _MERCURY_ID_MPZ_SIZE  4      /* each word is 4 bytes */

#undef min
#define min(a, b) (((a) < (b)) ? (a) : (b))

/* Highest bit set in a byte */
const char bytemsb[0x100] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};

/* Least set bit (ffs) */
const char bytelsb[0x100] = {
    0, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1,
    4, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1, 7, 1, 2, 1, 3, 1, 2, 1,
    4, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    6, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1,
    4, 1, 2, 1, 3, 1, 2, 1, 8, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1,
    4, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    7, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1,
    4, 1, 2, 1, 3, 1, 2, 1, 6, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
    5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1,
};

/* Find last set (most significant bit) */
static inline u_int fls32 (u_int32_t) __attribute__ ((const));
static inline u_int
fls32 (u_int32_t v)
{
    if (v & 0xffff0000) {
	if (v & 0xff000000)
	    return 24 + bytemsb[v>>24];
	else
	    return 16 + bytemsb[v>>16];
    }
    if (v & 0x0000ff00)
	return 8 + bytemsb[v>>8];
    else
	return bytemsb[v];
}

/* Ceiling of log base 2 */
static inline int log2c32 (u_int32_t) __attribute__ ((const));
static inline int
log2c32 (u_int32_t v)
{
    return v ? (int) fls32 (v - 1) : -1;
}

static inline u_int fls64 (u_int64_t) __attribute__ ((const));
static inline u_int
fls64 (u_int64_t v)
{
    u_int32_t h;
    if ((h = v >> 32))
	return 32 + fls32 (h);
    else
	return fls32 ((u_int32_t) v);
}

static inline int log2c64 (u_int64_t) __attribute__ ((const));
static inline int
log2c64 (u_int64_t v)
{
    return v ? (int) fls64 (v - 1) : -1;
}

#define fls(v) (sizeof (v) > 4 ? fls64 (v) : fls32 (v))
#define log2c(v) (sizeof (v) > 4 ? log2c64 (v) : log2c32 (v))

size_t
mpz_sizeinbase2 (const MP_INT *mp)
{
    size_t i;
    for (i = ABS (mp->_mp_size); i-- > 0;) {
	mp_limb_t v;
	if ((v = mp->_mp_d[i]))
	    return 8 * sizeof (mp_limb_t) * i + fls (v);
    }
    return 0;
}
void
_mpz_fixsize (MP_INT *r)
{
    mp_limb_t *sp = r->_mp_d;
    mp_limb_t *ep = sp + ABS (r->_mp_size);
    while (ep > sp && !ep[-1])
	ep--;
    r->_mp_size = r->_mp_size < 0 ? sp - ep : ep - sp;
    _mpz_assert (r);
}

void
mpz_umod_2exp (MP_INT *r, const MP_INT *a, u_long b)
{
    size_t nlimbs;
    const mp_limb_t *ap, *ae;
    mp_limb_t *rp, *re;

    if (a->_mp_size >= 0) {
	mpz_tdiv_r_2exp (r, a, b);
	return;
    }

    nlimbs = ((b + (8 * sizeof (mp_limb_t) - 1))
	      / (8 * sizeof (mp_limb_t)));
    if ((size_t) r->_mp_alloc < nlimbs)
	_mpz_realloc (r, nlimbs);
    ap = a->_mp_d;
    ae = ap + min ((size_t) ABS (a->_mp_size), nlimbs);
    rp = r->_mp_d;

    while (ap < ae)
	if ((*rp++ = -*ap++))
	    goto nocarry;
    r->_mp_size = 0;
    return;

 nocarry:
    while (ap < ae)
	*rp++ = ~*ap++;
    re = r->_mp_d + nlimbs;
    while (rp < re)
	*rp++ = ~(mp_limb_t) 0;
    re[-1] &= ~(mp_limb_t) 0 >> ((8*sizeof (mp_limb_t) - b)
				 % (8*sizeof (mp_limb_t)));
    r->_mp_size = nlimbs;
    _mpz_fixsize (r);
}

int
mpz_getbit (const MP_INT *mp, size_t bit)
{
    long limb = bit / mpz_bitsperlimb;
    long nlimbs = mp->_mp_size;
    if (mp->_mp_size >= 0) {
	if (limb >= nlimbs)
	    return 0;
	return mp->_mp_d[limb] >> bit % mpz_bitsperlimb & 1;
    }
    else {
	int carry;
	mp_limb_t *p, *e;

	nlimbs = -nlimbs;
	if (limb >= nlimbs)
	    return 1;

	carry = 1;
	p = mp->_mp_d;
	e = p + limb;
	for (; p < e; p++)
	    if (*p) {
		carry = 0;
		break;
	    }
	return (~*e + carry) >> bit % mpz_bitsperlimb & 1;
    }
}

#if SIZEOF_LONG < 8
void
mpz_set_u64 (MP_INT *mp, u_int64_t val)
{
    if (mp->_mp_alloc * sizeof (mp_limb_t) < 8)
	_mpz_realloc (mp, (8 + sizeof (mp_limb_t) - 1) / sizeof (mp_limb_t));
    u_int i = 0;
    while (val) {
	mp->_mp_d[i++] = val;
	val >>= 8 * sizeof (mp_limb_t);
    }
    mp->_mp_size = i;
}

void
mpz_set_s64 (MP_INT *mp, int64_t val)
{
    if (val < 0) {
	mpz_set_u64 (mp, -val);
	mp->_mp_size = -mp->_mp_size;
    }
    else
	mpz_set_u64 (mp, val);
}

u_int64_t
mpz_get_u64 (const MP_INT *mp)
{
    int i = ABS (mp->_mp_size);
    if (!i)
	return 0;
    u_int64_t ret = mp->_mp_d[--i];
    while (i > 0)
	ret = (ret << 8 * sizeof (mp_limb_t)) | mp->_mp_d[--i];
    if (mp->_mp_size > 0)
	return ret;
    return ~(ret - 1);
}

int64_t
mpz_get_s64 (const MP_INT *mp)
{
    int i = ABS (mp->_mp_size);
    if (!i)
	return 0;
    int64_t ret = mp->_mp_d[--i];
    while (i > 0)
	ret = (ret << 8 * sizeof (mp_limb_t)) | mp->_mp_d[--i];
    if (mp->_mp_size > 0)
	return ret;
    return ~(ret - 1);
}
#endif /* SIZEOF_LONG < 8 */

/* To be called from the debugger */
extern "C" void mpz_dump (const MP_INT *);
void
mpz_dump (const MP_INT *mp)
{
    char *str = (char *) malloc (mpz_sizeinbase (mp, 16) + 3);
    mpz_get_str (str, 16, mp);
    strcat (str, "\n");
    write (2, str, strlen (str));
    free (str);
}


#define mpz_get_rawmag mpz_get_rawmag_be
#define mpz_set_rawmag mpz_set_rawmag_be

#undef min
#define min(a, b) (((a) < (b)) ? (a) : (b))

static inline void
assert_limb_size ()
{
    switch (0) case 0: case GMP_LIMB_SIZE == sizeof (mp_limb_t):;
#if GMP_LIMB_SIZE != 2 && GMP_LIMB_SIZE != 4 && GMP_LIMB_SIZE != 8
# error Cannot handle size of GMP limbs
#endif /* GMP_LIMB_SIZE not 2, 4 or 8 */
}

#define COPYLIMB_BYTE(dst, src, SW, n) \
((char *) (dst))[n] = ((char *) (src))[SW (n)]

#if GMP_LIMB_SIZE == 2
# define COPYLIMB(dst, src, SW)		\
do {					\
    COPYLIMB_BYTE (dst, src, SW, 0);	\
	COPYLIMB_BYTE (dst, src, SW, 1);	\
} while (0)
#elif GMP_LIMB_SIZE == 4
# define COPYLIMB(dst, src, SW)		\
do {					\
    COPYLIMB_BYTE (dst, src, SW, 0);	\
	COPYLIMB_BYTE (dst, src, SW, 1);	\
	COPYLIMB_BYTE (dst, src, SW, 2);	\
	COPYLIMB_BYTE (dst, src, SW, 3);	\
} while (0)
#elif GMP_LIMB_SIZE == 8
# define COPYLIMB(dst, src, SW)		\
do {					\
    COPYLIMB_BYTE (dst, src, SW, 0);	\
	COPYLIMB_BYTE (dst, src, SW, 1);	\
	COPYLIMB_BYTE (dst, src, SW, 2);	\
	COPYLIMB_BYTE (dst, src, SW, 3);	\
	COPYLIMB_BYTE (dst, src, SW, 4);	\
	COPYLIMB_BYTE (dst, src, SW, 5);	\
	COPYLIMB_BYTE (dst, src, SW, 6);	\
	COPYLIMB_BYTE (dst, src, SW, 7);	\
} while (0)
#else /* GMP_LIMB_SIZE != 2, 4 or 8 */
# error Cannot handle size of GMP limbs
#endif /* GMP_LIMB_SIZE != 2, 4 or 8 */

#define LSWAP(n) ((n & -GMP_LIMB_SIZE) + GMP_LIMB_SIZE-1 - n % GMP_LIMB_SIZE)

#ifdef WORDS_BIGENDIAN
# define LE_POS(n) LSWAP(n)
# define BE_POS(n) n
#else /* !WORDS_BIGENDIAN */
# define LE_POS(n) n
# define BE_POS(n) LSWAP(n)
#endif /* !WORDS_BIGENDIAN */

void
mpz_get_rawmag_le (char *buf, size_t size, const MP_INT *mp)
{
    char *bp = buf;
    const mp_limb_t *sp = mp->_mp_d;
    const mp_limb_t *ep = sp + min (size / GMP_LIMB_SIZE,
				    (size_t) ABS (mp->_mp_size));

    while (sp < ep) {
	COPYLIMB (bp, sp, LE_POS);
	bp += GMP_LIMB_SIZE;
	sp++;
    }
    size_t n = size - (bp - buf);
    if (n < GMP_LIMB_SIZE && sp < mp->_mp_d + ABS (mp->_mp_size)) {
	mp_limb_t v = *sp;
	for (char *e = bp + n; bp < e; v >>= 8)
	    *bp++ = v;
    }
    else
	bzero (bp, n);
}

void
mpz_get_rawmag_be (char *buf, size_t size, const MP_INT *mp)
{
    char *bp = buf + size;
    const mp_limb_t *sp = mp->_mp_d;
    const mp_limb_t *ep = sp + min (size / GMP_LIMB_SIZE,
				    (size_t) ABS (mp->_mp_size));

    while (sp < ep) {
	bp -= GMP_LIMB_SIZE;
	COPYLIMB (bp, sp, BE_POS);
	sp++;
    }
    size_t n = bp - buf;
    if (n < GMP_LIMB_SIZE && sp < mp->_mp_d + ABS (mp->_mp_size)) {
	mp_limb_t v = *sp;
	for (; bp > buf; v >>= 8)
	    *--bp = v;
    }
    else
	bzero (buf, n);
}

void
mpz_get_raw (char *buf, size_t size, const MP_INT *mp)
{
    if (mp->_mp_size < 0) {
	mpz_t neg;
	mpz_init (neg);
	mpz_umod_2exp (neg, mp, size * 8);
	mpz_get_rawmag (buf, size, neg);
	mpz_clear (neg);
    }
    else
	mpz_get_rawmag (buf, size, mp);
}

void
mpz_set_rawmag_le (MP_INT *mp, const char *buf, size_t size)
{
    const char *bp = buf;
    size_t nlimbs = (size + sizeof (mp_limb_t)) / sizeof (mp_limb_t);
    mp_limb_t *sp;
    mp_limb_t *ep;

    mp->_mp_size = nlimbs;
    if (nlimbs > (u_long) mp->_mp_alloc)
	_mpz_realloc (mp, nlimbs);
    sp = mp->_mp_d;
    ep = sp + size / sizeof (mp_limb_t);

    while (sp < ep) {
	COPYLIMB (sp, bp, LE_POS);
	bp += GMP_LIMB_SIZE;
	sp++;
    }

    const char *ebp = buf + size;
    if (bp < ebp) {
	mp_limb_t v = (u_char) *--ebp;
	while (bp < ebp)
	    v = v << 8 | (u_char) *--ebp;
	*sp++ = v;
    }

    while (sp > mp->_mp_d && !sp[-1])
	sp--;
    mp->_mp_size = sp - mp->_mp_d;
}

void
mpz_set_rawmag_be (MP_INT *mp, const char *buf, size_t size)
{
    const char *bp = buf + size;
    size_t nlimbs = (size + sizeof (mp_limb_t)) / sizeof (mp_limb_t);
    mp_limb_t *sp;
    mp_limb_t *ep;

    mp->_mp_size = nlimbs;
    if (nlimbs > (u_long) mp->_mp_alloc)
	_mpz_realloc (mp, nlimbs);
    sp = mp->_mp_d;
    ep = sp + size / sizeof (mp_limb_t);

    while (sp < ep) {
	bp -= GMP_LIMB_SIZE;
	COPYLIMB (sp, bp, BE_POS);
	sp++;
    }

    if (bp > buf) {
	mp_limb_t v = (u_char) *buf++;
	while (bp > buf) {
	    v <<= 8;
	    v |= (u_char) *buf++;
	}
	*sp++ = v;
    }

    while (sp > mp->_mp_d && !sp[-1])
	sp--;
    mp->_mp_size = sp - mp->_mp_d;
}

void
mpz_set_raw (MP_INT *mp, const char *buf, size_t size)
{
    mpz_set_rawmag (mp, buf, size);
    if (*buf & 0x80) {
	mp->_mp_size = - mp->_mp_size;
	mpz_umod_2exp (mp, mp, 8 * size);
	mp->_mp_size = - mp->_mp_size;
    }
}

// all ids are restricted to be less than 
// 10240 bytes = 81920 bits long!

#define MAX_MPZ_SIZE  10240              
static byte sg_mpz_buffer [MAX_MPZ_SIZE];

MercuryID::MercuryID (Packet *pkt)
{
    uint32 size = (uint32) pkt->ReadInt();
    pkt->ReadBuffer (sg_mpz_buffer, size);
    mpz_init (this);
    mpz_set_raw (this, (const char *) sg_mpz_buffer, size);
}

void MercuryID::Serialize (Packet *pkt) 
{
    uint32 size = GetLength() - 4 /* subtract the size of the 'size' itself */;
    mpz_get_raw ((char*) sg_mpz_buffer, size, this);

    pkt->WriteInt ((int) size);
    pkt->WriteBuffer (sg_mpz_buffer, (int) size);
}

uint32 MercuryID::GetLength ()
{
    /* copied mpz_rawsize () from dm's code - Ashwin [02/13/2005] */
    
    uint32 nbits = mpz_sizeinbase2 (this);
    
    uint32 len = 4;             // the length itself
    if (nbits)
	len += (nbits>>3) + 1;	/* Not (nbits+7)/8, because we need sign bit */
    
    return len;
}

#define BIG_INT_NUM (1U << 20)

void MercuryID::double_mul (double d)
{
    u_long di = (u_long) (d * BIG_INT_NUM);

    mpz_mul_ui (this, this, di);
    mpz_tdiv_q_ui (this, this, BIG_INT_NUM);
}

/*
 * check if 'test' is between (left) and (right) considering the fact 
 * that values could be wrapped 
 */
bool IsBetweenInclusive (const Value& test, const Value& left, const Value& right)
{
    // why this first condition? 'left' and 'right' are both expected to 
    // be the left end-points of ranges. left == right => both are "same" 
    // so anything is in-between.  the situation we are considering here
    // is when my successor = myself!
    if (left == right)
	return true;

    if (left < right)
	return test >= left && test <= right;
    else
	return test >= left || test <= right;
}

bool IsBetweenLeftInclusive (const Value& test, const Value& left, const Value& right)
{
    // why this first condition? 'left' and 'right' are both expected to 
    // be the left end-points of ranges. left == right => both are "same" 
    // so anything is in-between.  the situation we are considering here
    // is when my successor = myself!
    if (left == right)
	return true;

    if (left < right)
	return test >= left && test < right;
    else
	return test >= left || test < right;
}

bool IsBetweenRightInclusive (const Value& test, const Value& left, const Value& right)
{
    // why this first condition? 'left' and 'right' are both expected to 
    // be the left end-points of ranges. left == right => both are "same" 
    // so anything is in-between.  the situation we are considering here
    // is when my successor = myself!
    if (left == right)
	return true;

    if (left < right)
	return test > left && test <= right;
    else
	return test > left || test <= right;
}
/*
 * check if 'test' is between (left) and (right) considering the fact 
 * that values could be wrapped 
 */
bool IsBetween (const Value& test, const Value& left, const Value& right)
{
    // why this first condition? 'left' and 'right' are both expected to 
    // be the left end-points of ranges. left == right => both are "same" 
    // so anything is in-between.  the situation we are considering here
    // is when my successor = myself!
    if (left == right)
	return true;

    if (left < right)
	return test > left && test < right;
    else
	return test > left || test < right;
}

#ifdef MERCURYID_PRINT_HEX
#define MERCURYID_PRINT_PAT "%ZX"
#else
#define MERCURYID_PRINT_PAT "%Zd"
#endif

void MercuryID::Print (FILE *stream)
{
    gmp_fprintf (stream, MERCURYID_PRINT_PAT, (MP_INT *)this);
}

ostream &operator<< (ostream &out, const MercuryID *id) 
{    
    char buf [256];
    gmp_sprintf (buf, MERCURYID_PRINT_PAT, (MP_INT *) id);
    return out << buf;
//    return out << (MP_INT *) (id);     // get rid of the gmpxx dependency at the cost of some
//    overhead (mostly debugging)
}

ostream &operator<< (ostream &out, const MercuryID &id)
{
    return out << &id;
}

/*
  int main()
  {
  MercuryID m = 219820938L;
  cout << m << endl;
  }
*/
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
