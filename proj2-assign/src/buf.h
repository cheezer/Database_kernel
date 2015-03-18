///////////////////////////////////////////////////////////////////////////////
/////////////  The Header File for the Buffer Manager /////////////////////////
///////////////////////////////////////////////////////////////////////////////


#ifndef BUF_H
#define BUF_H

#include "db.h"
#include "page.h"
#include "list"
#include "cmath"
#include "vector"
#include "set"
#include "queue"
#include "db.h"
#include "system_defs.h"
#include "new_error.h"

#define NUMBUF 20   
// Default number of frames, artifically small number for ease of debugging.

#define HTSIZE 7
// Hash Table size
//You should define the necessary classes and data structures for the hash table, 
// and the queues for LSR, MRU, etc.


/*******************ALL BELOW are purely local to buffer Manager********/

// You should create enums for internal errors in the buffer manager.
enum bufErrCodes  { NoMoreFrames, UnpinPageNotInBuffer, deletePinedPage
};

class Replacer;

class BufMgr {

private: // fill in this area
	int numbuf, ts;
	class HashTable{
	private:
		long long size, a, b;
		list<pair<PageId, int> >* hashList;
		int hash(PageId p)
		{
			return (a * (p % size) + b) % size;
		}

	public:
		HashTable (int numbuf)
		{
			for (size = numbuf; True; size++)
			{
				bool ok = true;
				for (int k = 2; k <= (int)sqrt(size); k++)
					if (size % k == 0)
					{
						ok = false;
						break;
					}
				if (ok)
					break;
			}
			hashList = new list<pair<PageId, int> >[size];
			a = rand() % size;
			b = rand() % size;
		}
		~HashTable()
		{
			delete [] hashList;
		}

		int find(PageId p)
		{
			int i = hash(p);
			for (list<pair<PageId, int> >::iterator it = hashList[i].begin(); it != hashList[i].end(); ++it)
				if (it ->first == p)
					return it -> second;
			return -1;
		}

		int put(PageId p, int f)
		{
			int i = hash(p);
			pair<PageId, int> kk = make_pair(p, f);
			hashList[i].push_back(kk);
			return 1;
		}

		int erase(PageId p)
		{
			int i = hash(p);
			for (list<pair<PageId, int> >::iterator it = hashList[i].begin(); it != hashList[i].end(); ++it)
				if (it ->first == p)
				{
					hashList[i].erase(it);
					return 1;
				}
			return -1;
		}
	} *hashTable;

	class Descriptor{
	public:
		PageId pageNum;
		int pinCount, dirty, loved, ts;
		Descriptor(PageId pageNum, int ts, int pc = 0, bool db = false, bool loved = false)
		{
			this->pageNum = pageNum;
			pinCount = pc;
			dirty = db;
			this->loved = loved;
			this->ts = ts;
		}
	} **bufDescr;

	class LHM{
	public:
		set<pair<int, PageId> > lovedPage;
		set<pair<int, PageId> > hatedPage;
		LHM()
		{
			//lovedPage = set<pair<int, PageId> >();
			//hatedPage = set<pair<int, PageId> >();
		}
		bool erase(PageId p, int ts, bool loved)
		{
			if (loved)
			{
				pair<int, PageId> pp = make_pair(ts, p);
				lovedPage.erase(pp);
			}
			else
			{
				pair<int, PageId> pp = make_pair(-ts, p);
				hatedPage.erase(pp);
			}
			return true;
		}
		bool put(PageId p, int ts, bool loved)
		{
			if (loved)
			{
				pair<int, PageId> pp = make_pair(ts, p);
				lovedPage.insert(pp);
			}
			else
			{
				pair<int, PageId> pp = make_pair(-ts, p);
				hatedPage.insert(pp);
			}
			return true;
		}

		int findVictim()
		{
			if (!hatedPage.empty())
			{
				return hatedPage.begin()->second;
			}
			else if (!lovedPage.empty())
			{
				return lovedPage.begin()->second;
			}
			else
				return -1;
		}
		int size()
		{
			return lovedPage.size() + hatedPage.size();
		}
	} *lhm;

public:

    Page* bufPool; // The actual buffer pool
    vector<int> freeFrames;

    BufMgr (int numbuf, Replacer *replacer = 0); 
    // Initializes a buffer manager managing "numbuf" buffers.
	// Disregard the "replacer" parameter for now. In the full 
  	// implementation of minibase, it is a pointer to an object
	// representing one of several buffer pool replacement schemes.

    ~BufMgr();           // Flush all valid dirty pages to disk

    Status pinPage(PageId PageId_in_a_DB, Page*& page, int emptyPage=0);
        // Check if this page is in buffer pool, otherwise
        // find a frame for this page, read in and pin it.
        // also write out the old page if it's dirty before reading
        // if emptyPage==TRUE, then actually no read is done to bring
        // the page

    Status unpinPage(PageId globalPageId_in_a_DB, int dirty, int hate);
        // hate should be TRUE if the page is hated and FALSE otherwise
        // if pincount>0, decrement it and if it becomes zero,
        // put it in a group of replacement candidates.
        // if pincount=0 before this call, return error.

    Status newPage(PageId& firstPageId, Page*& firstpage, int howmany=1); 
        // call DB object to allocate a run of new pages and 
        // find a frame in the buffer pool for the first page
        // and pin it. If buffer is full, ask DB to deallocate 
        // all these pages and return error

    Status freePage(PageId globalPageId); 
        // user should call this method if it needs to delete a page
        // this routine will call DB to deallocate the page 

    Status flushPage(PageId pageid);
        // Used to flush a particular page of the buffer pool to disk
        // Should call the write_page method of the DB class

    void deleteFrame(int i);

    Status flushAllPages();
	// Flush all pages of the buffer pool to disk, as per flushPage.

    /* DO NOT REMOVE THIS METHOD */    
    Status unpinPage(PageId globalPageId_in_a_DB, int dirty=FALSE)
        //for backward compatibility with the libraries
    {
      return unpinPage(globalPageId_in_a_DB, dirty, FALSE);
    }
    int freeSize()
    {
    	return freeFrames.size() + lhm -> size();
    }
};

#endif
