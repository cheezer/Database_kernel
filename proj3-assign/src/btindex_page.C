/*
 * btindex_page.cc - implementation of class BTIndexPage
 *
 */

#include "btindex_page.h"

// Define your Error Messge here
const char* BTIndexErrorMsgs[] = {
};
static error_string_table btree_table(BTINDEXPAGE, BTIndexErrorMsgs);

Status BTIndexPage::insertKey (const void *key,
                               AttrType key_type,
                               PageId pageNo,
                               RID& rid)
{ //assume there is space for this record
	int *length = new int;
	KeyDataEntry *target = new KeyDataEntry;
	Datatype datatype;
	datatype.pageNo = pageNo;
	make_entry(target, key_type, key, INDEX, datatype, length);
	Status s = SortedPage::insertRecord(key_type, (char *)target, *length, rid);
	delete length;
	delete target;
	if (s != OK)
	{
	   //MINIBASE_CHAIN_ERROR(BTREE, s);
	   return s;
	}
	return OK;
}

Status BTIndexPage::deleteKey (const void *key, AttrType key_type, RID& curRid)
{
  // put your code here
  return OK;
}
//left: <, right >=
Status BTIndexPage::get_page_no(const void *key,
                                AttrType key_type,
                                PageId & pageNo)
{
	Keytype nowKey;
	if (key_type == attrString)
		nowKey.charkey[0] = 0;
	else
		nowKey.intkey = 0;
	RID rid;
	PageId nowPageNo;
	Status s = get_first(rid, &nowKey, nowPageNo);
	int lastPageNo = getPrevPage();
	while (true)
	{
		if (s == NOMORERECS)
		{
			//MINIBASE_FIRST_ERROR(BTREE, s);
			pageNo = lastPageNo;
			return OK;
		}
		else if (s != OK)
		{
			MINIBASE_CHAIN_ERROR(BTREE, s);
			return s;
		}
		int cmp = keyCompare(key, &nowKey, key_type);
		if (cmp < 0)
		{
			pageNo = lastPageNo;
			return OK;
		}
		lastPageNo = nowPageNo;
		s = get_next(rid, &nowKey, nowPageNo);
	}
	return OK;
}

    
Status BTIndexPage::get_first(RID& rid,
                              void *key,
                              PageId & pageNo)
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
	get_key_data(key, &datatype, (KeyDataEntry *)recPtr, recLen, INDEX);
	pageNo = datatype.pageNo;
	return OK;
}

Status BTIndexPage::get_next(RID& rid, void *key, PageId & pageNo)
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
	get_key_data(key, &datatype, (KeyDataEntry *)recPtr, recLen, INDEX);
	pageNo = datatype.pageNo;
	return OK;

}
