/*
 * btfile.C - function members of class BTreeFile 
 * 
 */

#include "minirel.h"
#include "buf.h"
#include "db.h"
#include "new_error.h"
#include "btfile.h"
#include "btreefilescan.h"
#include "btleaf_page.h"
#include "vector"

// Define your error message here
const char* BtreeErrorMsgs[] = {
};

static error_string_table btree_table( BTREE, BtreeErrorMsgs);

BTreeFile::BTreeFile (Status& returnStatus, const char *filename)
{
	Status s = MINIBASE_DB->get_file_entry(filename, headerPageID);
	if (s != OK)
	{
		MINIBASE_CHAIN_ERROR(BTREE, s);
		return;
	}
	Page *page;
	MINIBASE_BM->pinPage(headerPageID, page);
	header = (BTreeHeaderPage*) page;
	this->filename = filename;
	destroied = false;
}

BTreeFile::BTreeFile (Status& returnStatus, const char *filename, 
                      const AttrType keytype,
                      const int keysize)
{
	Page* page;
	MINIBASE_BM->newPage(headerPageID, page);
	header = (BTreeHeaderPage*) page;
	header->keysize = keysize;
	header->keyType = keytype;
	MINIBASE_BM->newPage(header->root, page);
	BTLeafPage* leaf = (BTLeafPage *)page;
	leaf->init(header->root);
	leaf->setNextPage(-1);
	leaf->setPrevPage(-1);
	MINIBASE_BM->unpinPage(header->root, true);
	this->filename = filename;
	destroied = false;
}

BTreeFile::~BTreeFile ()
{
	if (destroied)
		return;
	MINIBASE_BM -> unpinPage(headerPageID, true);
	MINIBASE_DB -> add_file_entry(filename, headerPageID);
}

Status BTreeFile::destroy(PageId pageid)
{
	Page* page;
	MINIBASE_BM->pinPage(pageid, page);
	SortedPage* p = (SortedPage*) page;
	if (p->get_type() == INDEX)
	{
		RID rid;
		void *key = ((void *)new Keytype);
		PageId pageNo;
		Status s = ((BTIndexPage *)p) -> get_first(rid, key, pageNo);
		while (s != NOMORERECS)
		{
			if (s != OK)
			{
				MINIBASE_BM->unpinPage(pageid, false);
				MINIBASE_CHAIN_ERROR(BTREE, s);
				return s;
			}
			destroy(pageNo);
			s = ((BTIndexPage *)p) -> get_next(rid, key, pageNo);
		}
		delete (Keytype*) key;
	}
	Status s = MINIBASE_BM->unpinPage(pageid, false);
	if (s != OK)
	{
		MINIBASE_BM->unpinPage(pageid, false);
		MINIBASE_CHAIN_ERROR(BTREE, s);
		return s;
	}
	s = MINIBASE_BM -> freePage(pageid);
	if (s != OK)
	{
		MINIBASE_BM->unpinPage(pageid, false);
		MINIBASE_CHAIN_ERROR(BTREE, s);
		return s;
	}
	return OK;
}

Status BTreeFile::destroyFile ()
{
	destroied = true;
	Status s = destroy(header->root);
	MINIBASE_DB -> delete_file_entry(this->filename);
	if (s != OK)
	{
		MINIBASE_CHAIN_ERROR(BTREE, s);
		return s;
	}
	return OK;
}

Status BTreeFile::splitLeaf(const PageId pageid, BTLeafPage* leaf, void* &midKey, PageId &midPageID)
{
	Keytype key;
	RID rid, datarid;
	Status s = leaf->get_first(rid, &key, datarid);
	vector<pair<RID, pair<RID, Keytype> > > q;
	while (true)
	{
		if (s != NOMORERECS)
			q.push_back(make_pair(rid, make_pair(datarid, key)));
		else
			break;
		s = leaf->get_next(rid, &key, datarid);
	}
	Page* newPage;
	MINIBASE_BM->newPage(midPageID, newPage);
	BTLeafPage* newLeaf = (BTLeafPage*)newPage;
	newLeaf->init(midPageID);
	int llim = q.size() / 2;
	midKey = (void *)&q[llim].second.second;
	for (int k = q.size() - 1; k >= llim; k--)
	{
		leaf->deleteRecord(q[k].first);
		newLeaf->insertRec((void*)&q[k].second.second, header->keyType, q[k].second.first, rid);
	}
	newLeaf->setNextPage(leaf->getNextPage());
	leaf->setNextPage(midPageID);
	newLeaf->setPrevPage(pageid);
	MINIBASE_BM->unpinPage(midPageID, 1);
	Page* neNePage;
	MINIBASE_BM->pinPage(newLeaf->getNextPage(), neNePage);
	BTLeafPage* neNeLeaf = (BTLeafPage*)neNePage;
	neNeLeaf->setPrevPage(midPageID);
	MINIBASE_BM->unpinPage(newLeaf->getNextPage());
	return OK;
}

Status BTreeFile::splitIndex(const PageId pageid, BTIndexPage* index, void* &midKey, PageId &midPageID)
{
	Keytype key;
	RID rid;
	PageId dataPage;
	Status s = index->get_first(rid, &key, dataPage);
	vector<pair<RID, pair<PageId, Keytype> > > q;
	while (true)
	{
		if (s != NOMORERECS)
			q.push_back(make_pair(rid, make_pair(dataPage, key)));
		else
			break;
		s = index->get_next(rid, &key, dataPage);
	}
	Page* newPage;
	MINIBASE_BM->newPage(midPageID, newPage);
	BTIndexPage* newIndex = (BTIndexPage*)newPage;
	newIndex->init(midPageID);
	int llim = q.size() / 2;
	midKey = (void *)&q[llim].second.second;
	newIndex->setPrevPage(q[llim].second.first);
	for (int k = q.size() - 1; k > llim; k--)
	{
		index->deleteRecord(q[k].first);
		newIndex->insertKey((void*)&q[k].second.second, header->keyType, q[k].second.first, rid);
	}
	index->deleteRecord(q[llim].first);
	MINIBASE_BM->unpinPage(midPageID, 1);
	return OK;
}

Status BTreeFile::get(const PageId root, const void *key,
		RID& datarid, void* ridKey, PageId & ll, RID& rid)
{
	Page* page;
	MINIBASE_BM->pinPage(root, page);
	if (((SortedPage*)page)->get_type() == LEAF)
	{
		ll = root;
		BTLeafPage* leaf = (BTLeafPage*)page;
		Status s = leaf->get_geq_data_rid(key, header->keyType, datarid, ridKey, rid);
		MINIBASE_BM->unpinPage(root, 0);
		return s;
	}
	else{
		BTIndexPage* index = (BTIndexPage* )page;
		PageId p;
		Status s;
		if (key == NULL)
			s = get(index -> getPrevPage(), key, datarid, ridKey, ll, rid);
		else{
			index -> get_page_no(key, header->keyType, p);
			s = get(p, key, datarid, ridKey, ll, rid);
		}
		MINIBASE_BM->unpinPage(root, 0);
		return s;
	}
}

Status BTreeFile::printTree(const PageId root)
{
	Page* page;
	MINIBASE_BM->pinPage(root, page);
	if (((SortedPage*)page)->get_type() == LEAF)
	{
		cout << "PageID: " << root << " leaf"<< endl;
		BTLeafPage* leaf = (BTLeafPage*)page;
		RID rid, datarid;
		Keytype key;
		cout << "prev: " << leaf->getPrevPage() << endl;
		cout << "next: " << leaf->getNextPage() << endl;
		Status s = leaf -> get_first(rid, (void*)&key, datarid);
		while (s != NOMORERECS)
		{
			if (header->keyType == attrInteger)
				cout << "("<< rid.pageNo << "," << rid.slotNo << ") " << key.intkey << " ("<< datarid.pageNo << "," << datarid.slotNo << ") "  << endl;
			else
				cout << "("<< rid.pageNo << "," << rid.slotNo << ") " << key.charkey << " ("<< datarid.pageNo << "," << datarid.slotNo << ") " << endl;
			s = leaf-> get_next(rid, (void *) &key, datarid);
		}
		MINIBASE_BM->unpinPage(root, 0);
		cout << endl;
	}
	else{
		cout << "PageID: " << root << " index"<< endl;
		BTIndexPage* index = (BTIndexPage* )page;
		cout << "prev: " << index->getPrevPage() << endl;
		RID rid; PageId datarid;
		Keytype key;
		Status s = index -> get_first(rid, (void*)&key, datarid);
		while (s != NOMORERECS)
		{
			if (header->keyType == attrInteger)
				cout << "("<< rid.pageNo << "," << rid.slotNo << ") " << key.intkey << " " << datarid << endl;
			else
				cout << "("<< rid.pageNo << "," << rid.slotNo << ") " << key.charkey << " " << datarid << endl;
			s = index-> get_next(rid, (void *) &key, datarid);
		}
		cout << endl;
		printTree(index->getPrevPage());
		s = index -> get_first(rid, (void*)&key, datarid);
		while (s != NOMORERECS)
		{
			printTree(datarid);
			s = index-> get_next(rid, (void *) &key, datarid);
		}
		MINIBASE_BM->unpinPage(root, 0);
	}
	return OK;
}

Status BTreeFile::printTree()
{
	printTree(header->root);
}

Status BTreeFile::insertPage(const PageId root, const void *key, const RID datarid, void* &midKey, PageId &midPageID)
{
	Page* page;
	MINIBASE_BM->pinPage(root, page);
	midPageID = -1;
	if (((SortedPage*)page)->get_type() == LEAF)
	{
		BTLeafPage* leaf = (BTLeafPage*)page;
		RID rid;
		Status s = leaf->insertRec(key, header->keyType, datarid, rid);
		if (s != OK)
		{
			splitLeaf(root, leaf, midKey, midPageID);
			Status s;
			if (keyCompare(key, midKey, header->keyType) >= 0)
			{
				Page* p;
				MINIBASE_BM->pinPage(midPageID, p);
				BTLeafPage* newLeaf = (BTLeafPage*) p;
				s = newLeaf->insertRec(key, header->keyType, datarid, rid);
				MINIBASE_BM->unpinPage(midPageID);
			}
			else
				s = leaf->insertRec(key, header->keyType, datarid, rid);
			MINIBASE_BM->unpinPage(root, 1);
			if (s != OK)
			{
				MINIBASE_CHAIN_ERROR(BTREE, s);
				return s;
			}
			return OK;
		}
		else {
			MINIBASE_BM->unpinPage(root, 1);
			return OK;
		}
	}
	else {
		BTIndexPage* index = (BTIndexPage* )page;
		RID rid;
		Keytype kkk;
		void *nk = (void *)&kkk;
		if (header->keyType == attrString)
			((Keytype*) nk)->charkey[0] = 0;
		else
			((Keytype*) nk)->intkey = 0;
		PageId p, np;
		Status s = index -> get_page_no(key, header->keyType, p);
		insertPage(p, key, datarid, nk, np);
		if (np != -1)
		{
			Status s = index -> insertKey(nk, header ->keyType, np, rid);
			if (s != OK)
			{
				splitIndex(root, index, midKey, midPageID);

				if (keyCompare(key, midKey, header->keyType) >= 0)
				{
					Page* p;
					MINIBASE_BM->pinPage(midPageID, p);
					BTIndexPage* newIndex = (BTIndexPage*) p;
					s = newIndex->insertKey(key, header->keyType, np, rid);
					MINIBASE_BM->unpinPage(midPageID);
				}
				else
					s = index -> insertKey(nk, header ->keyType, np, rid);
				MINIBASE_BM->unpinPage(root, 1);
				//delete (Keytype*) nk;
				if (s != OK)
				{
					MINIBASE_CHAIN_ERROR(BTREE, s);
					return s;
				}
				return OK;
			}
			else{
				MINIBASE_BM->unpinPage(root, 1);
				//delete (Keytype*) nk; //TBC
				return OK;
			}
		}
		//delete (Keytype*) nk;
		MINIBASE_BM->unpinPage(root, 0);
		return OK;
	}
}

Status BTreeFile::insert(const void *key, const RID rid) {
	void* midKey;
	PageId midPageID;
	insertPage(header -> root, key, rid, midKey, midPageID);
	if (midPageID != -1)
	{
		PageId newRootID;
		Page* newPage;
		MINIBASE_BM->newPage(newRootID, newPage);
		BTIndexPage* newIndex = (BTIndexPage*)newPage;
		newIndex->init(newRootID);
		newIndex->setPrevPage(header->root);
		RID rid;
		newIndex->insertKey(midKey, header->keyType, midPageID, rid);
		MINIBASE_BM->unpinPage(newRootID);
		header->root = newRootID;
	}
	return OK;
}



Status BTreeFile::Delete(const void *key, const RID datarid) {
	RID r;
	void* rkey = (void*) new Keytype;
	PageId ll;
	RID rid;
	Status s = get(header->root, key, r, rkey, ll, rid);
	/*if (header->keyType == attrInteger)
		cout << ((Keytype *)rkey)->intkey << endl;
	else
		cout << ((Keytype *)rkey)->charkey << endl;*/
	if (s == OK && keyCompare(key, rkey, header->keyType) == 0)
	{
		delete (Keytype*)rkey;
		if (r == datarid)
		{
			Page* page;
			s = MINIBASE_BM->pinPage(ll, page);
			if (s != OK)
			{
				MINIBASE_CHAIN_ERROR(BTREE, s);
				return s;
			}
			BTLeafPage* leaf = (BTLeafPage*) page;
			leaf->deleteRecord(rid);
			s = MINIBASE_BM->unpinPage(ll);
			if (s != OK)
			{
				MINIBASE_CHAIN_ERROR(BTREE, s);
				return s;
			}
			return OK;
		}
		else
			return OK;
	}
	else
	{
		delete (Keytype*)rkey;
		return FAIL;
	}
}
    
IndexFileScan *BTreeFile::new_scan(const void *lo_key, const void *hi_key) {
	//printTree(header->root);
	void* rkey = (void*) new Keytype;
	PageId ll;
	RID rid, datarid;
	Status s = get(header->root, lo_key, datarid, rkey, ll, rid);
	BTreeFileScan* scan;
	if (s == OK)
		scan = new BTreeFileScan(header->keyType, header->keysize, rid, hi_key);
	else
	{
		rid.pageNo = -1;
		scan = new BTreeFileScan(header->keyType, header->keysize, rid, hi_key);
	}
	delete (Keytype*) rkey;
	return scan;
}
