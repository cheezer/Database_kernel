/*****************************************************************************/
/*************** Implementation of the Buffer Manager Layer ******************/
/*****************************************************************************/


#include "buf.h"


// Define buffer manager error messages here

// Define error message here
static const char* bufErrMsgs[] = { 
		"No More Frames",
		"Unpin Page Not In Buffer",
		"Delete Pined Page"
  // error message strings go here
};

// Create a static "error_string_table" object and register the error messages
// with minibase system 
static error_string_table bufTable(BUFMGR,bufErrMsgs);

BufMgr::BufMgr (int numbuf, Replacer *replacer) {
	this->numbuf = numbuf;
	bufPool = new Page[numbuf];
	hashTable = new HashTable(numbuf);
	for (int i = 0; i < numbuf; i++)
		freeFrames.push_back(i);
	bufDescr = new Descriptor* [numbuf];
	ts = 0;
	lhm = new LHM();
}

void BufMgr::deleteFrame(int i)
{
	if (bufDescr[i]->dirty)
		flushPage(bufDescr[i]->pageNum);
	lhm->erase(bufDescr[i]->pageNum, bufDescr[i]->ts, bufDescr[i]->loved);
	assert(hashTable->erase(bufDescr[i]->pageNum) == 1);
	delete bufDescr[i];
}

Status BufMgr::pinPage(PageId PageId_in_a_DB, Page*& page, int emptyPage) {
	int i = hashTable->find(PageId_in_a_DB);
	if (i != -1)
	{
		if (bufDescr[i]->pinCount == 0)
		{
			lhm->erase(PageId_in_a_DB, bufDescr[i]->ts, bufDescr[i]->loved);
		}
		bufDescr[i]->pinCount++;
	}
	else
	{
		if (!freeFrames.empty())
		{
			i = freeFrames.back();
			freeFrames.pop_back();
		}
		else {
			i = lhm->findVictim();
			if (i == -1)
			{
				MINIBASE_FIRST_ERROR(BUFMGR, FAIL);
				return FAIL;
			}
			i = hashTable->find(i);
			assert(i != -1);
			deleteFrame(i);
		}
		if (!emptyPage)
		{
			Status s = MINIBASE_DB->read_page(PageId_in_a_DB, &bufPool[i]);
			if (s != OK)
			{
				MINIBASE_CHAIN_ERROR(BUFMGR, s);
				freeFrames.push_back(i);
				return s;
			}
		}
		bufDescr[i] = new Descriptor(PageId_in_a_DB, ts, 1);
		hashTable->put(PageId_in_a_DB, i);
	}
	page = &bufPool[i];
	return OK;
}


Status BufMgr::newPage(PageId& firstPageId, Page*& firstpage, int howmany) {
	Status s = MINIBASE_DB->allocate_page(firstPageId, howmany);
	if (s != OK)
	{
		MINIBASE_CHAIN_ERROR(BUFMGR, s);
		return s;
	}
	s = pinPage(firstPageId, firstpage, 0);
	if (s != OK)
	{
		Status s2 = MINIBASE_DB->deallocate_page(firstPageId, howmany);
		MINIBASE_CHAIN_ERROR(BUFMGR, s2);
		MINIBASE_CHAIN_ERROR(BUFMGR, s);
		return s;
	}
	return OK;
}

Status BufMgr::flushPage(PageId pageid) {
	int i = hashTable->find(pageid);
	Status s = MINIBASE_DB->write_page(pageid, &bufPool[i]);
	if (s != OK)
	{
		MINIBASE_CHAIN_ERROR(BUFMGR, s);
		return s;
	}
	return OK;
}
    
	  
//*************************************************************
//** This is the implementation of ~BufMgr
//************************************************************
BufMgr::~BufMgr(){
	flushAllPages();
	int* z = new int[numbuf];
	for (int i = 0; i < numbuf; i++)
		z[i] = 1;
	for (int i = 0; i < freeFrames.size(); i++)
		z[freeFrames[i]] = 0;
	for (int i = 0; i < numbuf; i++)
		if (z[i])
			deleteFrame(i);
	delete [] bufDescr;
	delete [] bufPool;
	delete hashTable;
	delete lhm;
  // put your code here
}


//*************************************************************
//** This is the implementation of unpinPage
//************************************************************

Status BufMgr::unpinPage(PageId page_num, int dirty=FALSE, int hate = FALSE){
	int i = hashTable->find(page_num);
	ts++;
	if (i == -1)
	{
		MINIBASE_FIRST_ERROR(BUFMGR, FAIL);
		return FAIL;
	}
	else if (bufDescr[i]->pinCount == 0)
	{
		MINIBASE_FIRST_ERROR(BUFMGR, FAIL);
		return FAIL;
	}
	else
	{
		bufDescr[i]->ts = ts;
		bufDescr[i]->dirty |= dirty;
		bufDescr[i]->loved |= !hate;
		bufDescr[i]->pinCount--;
		if (bufDescr[i]->pinCount == 0)
		{
			lhm->put(page_num, bufDescr[i]->ts, bufDescr[i]->loved);
		}
	}
	return OK;
}

//*************************************************************
//** This is the implementation of freePage
//************************************************************

Status BufMgr::freePage(PageId globalPageId){
	int i = hashTable->find(globalPageId);
	if (i != -1)
	{
		if (bufDescr[i]->pinCount > 0)
		{
			MINIBASE_FIRST_ERROR(BUFMGR, FAIL);
			return FAIL;
		}
		freeFrames.push_back(i);
	}
	Status s = MINIBASE_DB->deallocate_page(globalPageId);
	if (s != OK)
	{
		MINIBASE_CHAIN_ERROR(BUFMGR, s);
		return s;
	}
	return OK;
}

Status BufMgr::flushAllPages(){
	int* z = new int[numbuf];
	for (int i = 0; i < numbuf; i++)
		z[i] = 1;
	for (int i = 0; i < freeFrames.size(); i++)
		z[freeFrames[i]] = 0;
	for (int i = 0; i < numbuf; i++)
		if (z[i] && bufDescr[i]->dirty)
		{
			Status s = flushPage(bufDescr[i]->pageNum);
			if (s != OK)
			{
				MINIBASE_CHAIN_ERROR(BUFMGR, s);
				return s;
			}
		}
  return OK;
}
