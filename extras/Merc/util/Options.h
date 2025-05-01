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
/* -*- Mode:c++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */

#ifndef __OPTIONS__H
#define __OPTIONS__H

/*
 * Generic option processing code. I think this is more generic than getopt.
 * You don't have to write the getopt while loop again and again and again...
 * Furthermore, all options stay documented the way they are inserted into the
 * options_struct argument - ashwin
 */

// Option flags; OPT_NOARG can be combined with any of the other flags. 
#define OPT_NOARG        0x01       // The option does not take any argument
#define OPT_INT          0x02       // The value of this option is an integer
#define OPT_STR          0x04       // string
#define OPT_FLT          0x08       // float
#define OPT_CHR          0x10       // character
#define OPT_BOOL         0x20       // bool (just for convenience sake!)
#define OPT_DBL          0x40       // double (sometimes we need more precision!)

// Definition of an option
typedef struct {
    char shortletter;      // short option
    char *longword;        // long option
    int  flags;            // [OPT_NOARG] | {OPT_<type>}
    char *comment;         // documentation of the option - for PrintUsage()
    void *varptr;          // pointer to the location which stores this option's value
    char *default_val;     // default value expressed in "string" form
    void *val_to_set;      // [only for options which don't take any arguments]
    // specifies the value to be set when the option is given
} OptionType;

extern "C" { 
    /**
     * merge the list of options to create one large options array
     **/
    OptionType *MergeOptions (OptionType *optsarray[], int alen);
    int  ProcessOptions(OptionType *options, int argc, char *argv[], bool complain);
    // Process options. Changes the argv[] array.
    void PrintUsage(OptionType *options);           // Print option documentation 
    void PrintOptionValues(OptionType *options);    // Print the options values currently set

};

#endif // __OPTIONS__H
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
