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

#include "Options.h"
#include <util/debug.h>
#include <vector>
using namespace std;

void PrintUsage(OptionType * options_def)
{
    OptionType *ptr;

    fprintf(stderr, "Usage: ProgramName [options]:\n");
    unsigned int maxlen = 0;
    for (ptr = options_def; ptr->longword; ptr++) 
	if (strlen(ptr->longword) > maxlen)
	    maxlen = strlen(ptr->longword);

    for (ptr = options_def; ptr->longword; ptr++) {
	fprintf(stderr, "\t-%c,--%s:", ptr->shortletter,
		ptr->longword);
	for (unsigned int i = 0; i <= maxlen - strlen (ptr->longword); i++)
	    fprintf (stderr, " ");
	fprintf (stderr, "%s\n", ptr->comment);
    }
    fprintf(stderr, "\n");
}

void _AssignValue(OptionType * ptr, char *value)
{
    if (ptr->flags & OPT_CHR) {
	*(char *) ptr->varptr = *value;
    } else if (ptr->flags & OPT_INT) {
	*(int *) ptr->varptr = atoi(value);
    } else if (ptr->flags & OPT_FLT) {
	*(float *) ptr->varptr = (float) atof(value);
    } else if (ptr->flags & OPT_DBL) {
	*(double *) ptr->varptr = atof(value);		
    } else if (ptr->flags & OPT_STR) {
	// XXX security risk!! change to strncpy, but then OPT_STR needs 
	// an additional parameter for max-size.. perhaps, ptr->val_to_set 
	// can be used hackily
	strcpy((char *) ptr->varptr, value);
    } else if (ptr->flags & OPT_BOOL) {
	bool toSet = (strcmp(value, "1") == 0 ? true : false);
	*((bool *) ptr->varptr) = toSet;
    }
}

void PrintOptionValues(OptionType * options_def)
{
    OptionType *ptr;

    unsigned int maxlen = 0;
    for (ptr = options_def; ptr->longword; ptr++) 
	if (strlen(ptr->longword) > maxlen)
	    maxlen = strlen(ptr->longword);

    fprintf(stderr, "Current option values...\n");
    for (ptr = options_def; ptr->longword; ptr++) {
	fprintf(stderr, "\t-%c,--%s:", ptr->shortletter, ptr->longword);
	for (unsigned int i = 0; i <= maxlen - strlen (ptr->longword); i++)
	    fprintf (stderr, " ");
	if (ptr->flags & OPT_CHR)
	    fprintf(stderr, "%c\n", *(char *) ptr->varptr);
	else if (ptr->flags & OPT_INT)
	    fprintf(stderr, "%d\n", *(int *) ptr->varptr);
	else if (ptr->flags & OPT_FLT)
	    fprintf(stderr, "%f\n", *(float *) ptr->varptr);
	else if (ptr->flags & OPT_DBL)
	    fprintf(stderr, "%lf\n", *(double *) ptr->varptr);
	else if (ptr->flags & OPT_STR)
	    fprintf(stderr, "%s\n", (char *) ptr->varptr);
	else if (ptr->flags & OPT_BOOL)
	    fprintf(stderr, "%s\n",
		    (*((bool *) ptr->varptr) == true ? "true" : "false"));
    }
    fprintf(stderr, "\n");
}

void _AssignDefaults(OptionType * options_def)
{
    OptionType *ptr;

    for (ptr = options_def; ptr->longword; ptr++) {
	_AssignValue(ptr, ptr->default_val);
    }
}

//
// Process the options defined by 'options_def'
//   The function changes the argv[] array by removing all
//                 the options that were processed.
//
int ProcessOptions(OptionType * options_def, int argc, char *argv[],
		   bool complain)
{
    int i = 1;
    char sbuf[3], lbuf[24];
    OptionType *ptr;
    vector < int >notprocessed;

    sbuf[0] = '-';
    sbuf[2] = 0;
    lbuf[0] = lbuf[1] = '-';

    _AssignDefaults(options_def);

    while (i < argc) {
	ptr = options_def;
	if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help") ||
	    !strcmp(argv[i], "--h")) {
	    PrintUsage(options_def);
	    exit(0);
	}
	while (ptr->longword) {
	    sbuf[1] = ptr->shortletter;
	    strcpy(lbuf + 2, ptr->longword);

	    if (!strcmp(argv[i], sbuf) || !strcmp(argv[i], lbuf)) {
		//
		// Process this option now.
		//
		if (ptr->flags & OPT_NOARG) {
		    _AssignValue(ptr,
				 (ptr->val_to_set ? (char *) ptr->
				  val_to_set : (char *) "1"));
		    i += 1;
		} else {
		    int index = i + 1;
		    ASSERT(index < argc);
		    //fprintf(stderr, "%s\n", argv[index]);
		    _AssignValue(ptr, argv[index]);
		    i += 2;
		}
		break;
	    }
	    ptr++;
	}
	if (!ptr->longword) {
	    if (complain) {
		cerr << " *** unknown option: " << argv[i] << endl;
		// PrintUsage(options_def);
		// fprintf(stderr, "***WARNING*** Invalid argument: %s\n",
		//		argv[i]);
	    }

	    notprocessed.push_back(i);
	    i++;
	    continue;
	}
    }


    /* Now, remove all the processed elements from argv and update argc accordingly. */
    for (int j = 0; j < (int) notprocessed.size(); j++) {
	argv[j + 1] = argv[notprocessed[j]];	// in memory copying. but works. (small := large)
    }
    return notprocessed.size() + 1;
}

static int GetLength (OptionType *ptr)
{
    int ret = 0;
    while (ptr->longword) {
	ret++;
	ptr++;
    }		
    return ret;
}

OptionType *MergeOptions (OptionType *optsarray[], int alen)
{
    int total = 0;
    for (int i = 0; i < alen; i++) {
	total += GetLength (optsarray[i]);
    }
    total += 1;

    OptionType *ret = new OptionType[total];
    int cur = 0;
    for (int i = 0; i < alen; i++) {
	OptionType *ptr = optsarray[i];
	while (ptr->longword) {
	    ret[cur++] = *ptr;
	    ptr++;
	}
    }
    memset (&ret[total - 1], 0, sizeof (OptionType));

    return ret;
}
// vim: set sw=4 sts=4 ts=8 noet: 
// Local Variables:
// Mode: c++
// c-basic-offset: 4
// tab-width: 8
// indent-tabs-mode: t
// End:
