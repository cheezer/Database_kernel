/*
 * btreefilescan.cc - function members of class BTreeFileScan
 *
 */

#include "minirel.h"
#include "buf.h"
#include "db.h"
#include "new_error.h"
#include "btfile.h"
#include "btreefilescan.h"

/*
 * Note: BTreeFileScan uses the same errors as BTREE since its code basically 
 * BTREE things (traversing trees).
 */
BTreeFileScan::BTreeFileScan(AttrType t, int keysize, RID r, const void* up)
{
	keyR = up;
	ks = keysize;
	currentRID = r;
	currentRID.slotNo--;
	delable = false;
	type = t;
}

BTreeFileScan::~BTreeFileScan ()
{
}

int BTreeFileScan::keysize() 
{
	return ks;
}

Status BTreeFileScan::get_next (RID & rid, void* keyptr)
{
	if (currentRID.pageNo == -1)
		return DONE;
	Page* page;
	MINIBASE_BM->pinPage(currentRID.pageNo, page);
	BTLeafPage* leaf = (BTLeafPage*)page;
	Status s;
	while (true)
	{
		if (currentRID.slotNo == -1)
		{
			s = leaf->get_first(currentRID, keyptr, rid);
			if (keyR != NULL && keyCompare(keyptr, keyR, type) > 0)
			{
				MINIBASE_BM->unpinPage(currentRID.pageNo);
				return DONE;
			}
		}
		else
		{
			s = leaf->get_next(currentRID, keyptr, rid);
			if (keyR != NULL && keyCompare(keyptr, keyR, type) > 0)
			{
				MINIBASE_BM->unpinPage(currentRID.pageNo);
				return DONE;
			}
		}
		if (s == NOMORERECS)
		{
			RID nextRID;
			nextRID.pageNo = leaf->getNextPage();
			nextRID.slotNo = -1;
			MINIBASE_BM->unpinPage(currentRID.pageNo);
			currentRID = nextRID;
			if (currentRID.pageNo == -1)
				return DONE;
			MINIBASE_BM->pinPage(currentRID.pageNo, page);
			leaf = (BTLeafPage*)page;
		}
		else break;
	}
	delable = true;
	return OK;
}

Status BTreeFileScan::delete_current ()
{
	if (!delable)
		return FAIL;
	Page* page;
	MINIBASE_BM->pinPage(currentRID.pageNo, page);
	BTLeafPage* leaf = (BTLeafPage*)page;
	delable = false;
	leaf->deleteRecord(currentRID);
	currentRID.slotNo--;
	MINIBASE_BM->unpinPage(currentRID.pageNo);
	return OK;
}
