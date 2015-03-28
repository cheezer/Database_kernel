/*
 * btleaf_page.cc - implementation of class BTLeafPage
 *
 */

#include "btleaf_page.h"

const char* BTLeafErrorMsgs[] = {
};
static error_string_table btree_table(BTLEAFPAGE, BTLeafErrorMsgs);
   
Status BTLeafPage::insertRec(const void *key,
                              AttrType key_type,
                              RID dataRid,
                              RID& rid)
{
	RID tRID; const void *tkey = key;
	Status s = get_data_rid(tkey, key_type, tRID);
	if (s == OK)
		return OK;
	int *length = new int;
	KeyDataEntry * target = new KeyDataEntry;
	Datatype datatype;
	datatype.rid = dataRid;
	make_entry(target, key_type, key, LEAF, datatype, length);
	s = SortedPage::insertRecord(key_type, (char *)target, *length, rid);
	delete length;
	delete target;
	if (s != OK)
	{
	   //MINIBASE_CHAIN_ERROR(BTREE, s);
	   return s;
	}
	return OK;
}

Status BTLeafPage::get_data_rid(const void *key,
                                AttrType key_type,
                                RID & dataRid)
{
	Keytype nowKey;
	if (key_type == attrString)
		nowKey.charkey[0] = 0;
	else
		nowKey.intkey = 0;
	RID rid;
	Status s = get_first(rid, &nowKey, dataRid);
	while (true)
	{
		if (s == NOMORERECS)
		{
			//MINIBASE_FIRST_ERROR(BTREE, s);
			return FAIL;
		}
		else if (s != OK)
		{
			MINIBASE_CHAIN_ERROR(BTREE, s);
			return s;
		}
		int cmp = keyCompare(key, &nowKey, key_type);
		if (cmp == 0)
		{
			return OK;
		}
		else if (cmp < 0)
		{
			return FAIL; //not found
		}
		s = get_next(rid, &nowKey, dataRid);
	}
	return OK;
}

Status BTLeafPage::get_geq_data_rid(const void *key,
                                AttrType key_type,
                                RID & dataRid, void *rKey, RID& rid)
{
	if (key_type == attrString)
		((Keytype*)rKey)->charkey[0] = 0;
	else
		((Keytype*)rKey)->intkey = 0;
	Status s = get_first(rid, rKey, dataRid);
	while (true)
	{
		if (s == NOMORERECS)
		{
			return FAIL;
		}
		else if (s != OK)
		{
			MINIBASE_CHAIN_ERROR(BTREE, s);
			return s;
		}
		int cmp;
		if (key != NULL)
			cmp = keyCompare(key, rKey, key_type);
		else cmp = -1;
		if (cmp <= 0)
		{
			return OK;
		}
		s = get_next(rid, rKey, dataRid);
	}
	return OK;
}

Status BTLeafPage::get_first (RID& rid,
                              void *key,
                              RID & dataRid)
{ 
	//assume keys are newed
	Status s = SortedPage::firstRecord(rid);
	if (s == DONE)
	{
		return NOMORERECS;
	}
	else if (s != OK)
	{
	   MINIBASE_CHAIN_ERROR(BTREE, s);
	   return s;
	}
	char* recPtr;
	int recLen;
	s = SortedPage::returnRecord(rid, recPtr, recLen);
	if (s != OK)
	{
	   MINIBASE_CHAIN_ERROR(BTREE, s);
	   return s;
	}
	Datatype datatype;
	get_key_data(key, &datatype, (KeyDataEntry *)recPtr, recLen, LEAF);
	dataRid = datatype.rid;
	return OK;
}

Status BTLeafPage::get_next (RID& rid,
                             void *key,
                             RID & dataRid)
{
	RID oriRid = rid;
	Status s = SortedPage::nextRecord(oriRid, rid);
	if (s == DONE)
	{
		return NOMORERECS;
	}
	else if (s != OK)
	{
	   MINIBASE_CHAIN_ERROR(BTREE, s);
	   return s;
	}
	char* recPtr;
	int recLen;
	s = SortedPage::returnRecord(rid, recPtr, recLen);
	if (s != OK)
	{
	   MINIBASE_CHAIN_ERROR(BTREE, s);
	   return s;
	}
	Datatype datatype;
	get_key_data(key, &datatype, (KeyDataEntry *)recPtr, recLen, LEAF);
	dataRid = datatype.rid;
	return OK;
}
