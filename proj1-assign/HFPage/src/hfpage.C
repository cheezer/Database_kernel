#include <iostream>
#include <stdlib.h>
#include <memory.h>

#include "hfpage.h"
#include "heapfile.h"
#include "buf.h"
#include "db.h" 


// **********************************************************
// page class constructor

void HFPage::init(PageId pageNo)
{
    prevPage = INVALID_PAGE;
    nextPage = INVALID_PAGE;
    curPage = pageNo;
    slotCnt = 0;
    freeSpace = sizeof(data) + sizeof(slot_t);
    usedPtr = sizeof(data);//the last one + 1
    slot[0].length = EMPTY_SLOT;
  // fill in the body
}

// **********************************************************
// dump page utlity
void HFPage::dumpPage()
{
    int i;

    cout << "dumpPage, this: " << this << endl;
    cout << "curPage= " << curPage << ", nextPage=" << nextPage << endl;
    cout << "usedPtr=" << usedPtr << ",  freeSpace=" << freeSpace
         << ", slotCnt=" << slotCnt << endl;
   
    for (i=0; i < slotCnt; i++) {
        cout << "slot["<< i <<"].offset=" << slot[i].offset
             << ", slot["<< i << "].length=" << slot[i].length << endl;
    }
}

// **********************************************************
PageId HFPage::getPrevPage()
{
    // fill in the body
    return prevPage;
}

// **********************************************************
void HFPage::setPrevPage(PageId pageNo)
{
    prevPage = pageNo;
    // fill in the body
}

// **********************************************************
void HFPage::setNextPage(PageId pageNo)
{
    nextPage = pageNo;
  // fill in the body
}

// **********************************************************
PageId HFPage::getNextPage()
{
    // fill in the body
    return nextPage;
}

// **********************************************************
// Add a new record to the page. Returns OK if everything went OK
// otherwise, returns DONE if sufficient space does not exist
// RID of the new record is returned via rid parameter.
Status HFPage::insertRecord(char* recPtr, int recLen, RID& rid)
{
    // fill in the body
    int select = -1, i;
    for (i=0; i < slotCnt; i++) {
        if (slot[i].length == EMPTY_SLOT)
        {
            select = i;
            break;
        }
    }
    if (((select == -1) && (freeSpace < recLen + sizeof(slot_t))) || ((select != -1) && (freeSpace < recLen)))
        return DONE;
    if (select == -1)
    {
        i = slotCnt;
        slot[i].length = recLen;
        slot[i].offset = usedPtr - 1;
        slotCnt++;
        freeSpace -= sizeof(slot_t);
        rid.slotNo = i;
    }
    else
    {
        slot[select].length = recLen;
        slot[select].offset = usedPtr - 1;
        rid.slotNo = select;
    }
    rid.pageNo = curPage;
    int k;
    for (k = 1; k <= recLen; ++k)
        data[usedPtr - k] = *(recPtr + k - 1);
    usedPtr -= recLen;
    freeSpace -= recLen;
    return OK;
}

// **********************************************************
// Delete a record from a page. Returns OK if everything went okay.
// Compacts remaining records but leaves a hole in the slot array.
// Use memmove() rather than memcpy() as space may overlap.
Status HFPage::deleteRecord(const RID& rid)
{
    // fill in the body
    if (!(rid.pageNo == curPage))
        return FAIL;
    if (rid.slotNo >= slotCnt || rid.slotNo < 0 || slot[rid.slotNo].length == EMPTY_SLOT)
        return FAIL;
    int i = rid.slotNo, j = slot[i].length, k;
    memmove(data + usedPtr + j, data + usedPtr, slot[i].offset - usedPtr + 1 - j);
    slot[i].length = EMPTY_SLOT;
    for (k = 0; k < slotCnt; k++)
        if (slot[k].length != EMPTY_SLOT && slot[k].offset <= slot[i].offset)
            slot[k].offset += j;
    usedPtr += j;
    freeSpace += j;
    if (i == slotCnt - 1)
    {
        slotCnt--;
        freeSpace += sizeof(slot_t);
    }
    while (slotCnt > 0 && slot[slotCnt - 1].length == EMPTY_SLOT)
    {
        slotCnt--;
        freeSpace += sizeof(slot_t);
    }
    return OK;
}

// **********************************************************
// returns RID of first record on page
Status HFPage::firstRecord(RID& firstRid)
{
    // fill in the body
    int i;
    for (i = 0; i < slotCnt; ++i)
        if (slot[i].length != EMPTY_SLOT)
        {
            firstRid.pageNo = curPage;
            firstRid.slotNo = i;
            return OK;
        }
    return DONE;
}

// **********************************************************
// returns RID of next record on the page
// returns DONE if no more records exist on the page; otherwise OK
Status HFPage::nextRecord (RID curRid, RID& nextRid)
{
    // fill in the body
    if (curRid.pageNo != curPage || curRid.slotNo < 0 || curRid.slotNo >= slotCnt || slot[curRid.slotNo].length == EMPTY_SLOT)
        return FAIL;
    int i;
    for (i = curRid.slotNo + 1; i < slotCnt; ++i)
        if (slot[i].length != EMPTY_SLOT)
        {
            nextRid.pageNo = curPage;
            nextRid.slotNo = i;
            return OK;
        }
    return DONE;
}

// **********************************************************
// returns length and copies out record with RID rid
Status HFPage::getRecord(RID rid, char* recPtr, int& recLen)
{
    if (rid.pageNo != curPage || rid.slotNo < 0 || rid.slotNo >= slotCnt || slot[rid.slotNo].length == EMPTY_SLOT)
        return FAIL;
    int i, num = rid.slotNo;
    for (i = 0; i < slot[num].length; ++i)
        recPtr[i] = data[slot[num].offset - i];
    recLen = slot[num].length;
    // fill in the body
    return OK;
}

// **********************************************************
// returns length and pointer to record with RID rid.  The difference
// between this and getRecord is that getRecord copies out the record
// into recPtr, while this function returns a pointer to the record
// in recPtr.
Status HFPage::returnRecord(RID rid, char*& recPtr, int& recLen)
{
    if (rid.pageNo != curPage || rid.slotNo >= slotCnt || rid.slotNo < 0 || slot[rid.slotNo].length == EMPTY_SLOT)
        return FAIL;
    int i, num = rid.slotNo;
    recPtr = new char[slot[num].length];
    for (i = 0; i < slot[num].length; ++i)
        recPtr[i] = data[slot[num].offset - i];
    recLen = slot[num].length;
    // fill in the body
    return OK;
}

// **********************************************************
// Returns the amount of available space on the heap file page
int HFPage::available_space(void)
{
    // fill in the body
    return freeSpace - sizeof(slot);
}

// **********************************************************
// Returns 1 if the HFPage is empty, and 0 otherwise.
// It scans the slot directory looking for a non-empty slot.
bool HFPage::empty(void)
{
    int i;
    for (i = 0; i < slotCnt; ++i)
        if (slot[i].length != EMPTY_SLOT)
        {
            return 0;
        }
    // fill in the body
    return 1;
}



