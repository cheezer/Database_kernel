/*
 * key.C - implementation of <key,data> abstraction for BT*Page and 
 *         BTreeFile code.
 *
 */

#ifndef _KEY_H
#define _KEY_H

#include <string.h>
#include <assert.h>

#include "bt.h"

/*
 * See bt.h for more comments on the functions defined below.
 */

/*
 * Reminder: keyCompare compares two keys, key1 and key2
 * Return values:
 *   - key1  < key2 : negative
 *   - key1 == key2 : 0
 *   - key1  > key2 : positive
 */

int keyCompare(const void *key1, const void *key2, AttrType t);

int get_key_length(const void *key, const AttrType key_type);
    
int get_key_data_length(const void *key, const AttrType key_type, 
                        const nodetype ndtype);

 
/*
 * fill_entry_key: used by make_entry (below) to write the <key> part
 * of a KeyDataEntry (*target, that is). 
 * Returns <key> part length in *pentry_key_len.
 */ 

/*static*/ void fill_entry_key(Keytype *target,
                           const void *key, AttrType key_type,
                           int *pentry_key_len);

/*
 * fill_entry_data: writes <data> part to a KeyDataEntry (into which
 * `target' points).  Returns length of <data> part in *pentry_data_len.
 *
 * Note that these do memcpy's instead of direct assignments because 
 * `target' may not be properly aligned.
 */

/*static*/ void fill_entry_data(char *target,
                            Datatype source, nodetype ndtype,
                            int *pentry_data_len);
    
/*
 * make_entry: write a <key,data> pair to a blob of memory (*target) big
 * enough to hold it.  Return length of data in the blob via *pentry_len.
 *
 * Ensures that <data> part begins at an offset which is an even 
 * multiple of sizeof(PageNo) for alignment purposes.
 */

void make_entry(KeyDataEntry *target,
                AttrType key_type, const void *key,
                nodetype ndtype, Datatype data,
                int *pentry_len);


/*
 * get_key_data: unpack a <key,data> pair into pointers to respective parts.
 * Needs a) memory chunk holding the pair (*psource) and, b) the length
 * of the data chunk (to calculate data start of the <data> part).
 */

void get_key_data(void *targetkey, Datatype *targetdata,
                  KeyDataEntry *psource, int entry_len, nodetype ndtype);
#endif
