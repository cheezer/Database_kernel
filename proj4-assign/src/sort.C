// file sort.C  --  implementation of Sort class.

#include "sort.h"
#include "heapfile.h"
#include "system_defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <ios>
#include <cstring>

TupleOrder Sort::sortOrder;
int Sort::fld_no, Sort::fieldLength, Sort::offset;
AttrType Sort::attrType;

// Add you error message here
// static const char *sortErrMsgs[] {
// };
// static error_string_table sortTable(SORT, sortErrMsgs);

int Sort::sortCmp(const void *pRec1, const void *pRec2)
{
  int result;

  char *rec1 = (char *)pRec1;
  char *rec2 = (char *)pRec2;

  if (Sort::attrType == attrInteger)
	result = *(int *)(rec1 + Sort::offset) - *(int *)(rec2 + Sort::offset);
  else
	result = (strncmp(&rec1[Sort::offset], &rec2[Sort::offset], Sort::fieldLength));

  if (result < 0)
	if (Sort::sortOrder == Ascending)
	  return -1;
	else
	  return 1;
  else
	if (result > 0)
	  if (Sort::sortOrder == Ascending)
		return 1;
	  else
		return -1;
	else
	  return 0;
}

// constructor.

Sort::Sort ( char* inFile, char* outFile, int len_in, AttrType in[],
	     short str_sizes[], int fld_no, TupleOrder sort_order,
	     int amt_of_buf, Status& s )
{
	outFileName = outFile;
	bufNum = amt_of_buf;
	recSize = 0;
	for (int i = 0; i < len_in; i++)
	{
		if (i == fld_no)
		{
			offset = recSize;
			fieldLength = str_sizes[i];
		}
		recSize += str_sizes[i];
	}
	attrType = in[fld_no];
	sortOrder = sort_order;
	int HFNum, runNum = 1;
	firstPass(inFile, bufNum, HFNum);
	while (HFNum > 1)
		followingPass(++runNum, HFNum, bufNum, HFNum);
	HeapFile out(outFile, s);
	makeHFname(HFname, runNum, 1);
	HeapFile sortFile(HFname, s);
	Scan* scan = sortFile.openScan(s);
	char* res = new char[recSize];
	RID rid;
	int len;
	for (s = scan->getNext(rid, res, len); s == OK; s = scan->getNext(rid, res, len))
		out.insertRecord(res, recSize, rid);
	delete res;
	sortFile.deleteFile();
	s = OK;
}

void Sort::makeHFname( char *name, int passNum, int HFnum )
{
	sprintf(name, "Run%d_%d.txt", passNum, HFnum);
}

// first pass.
Status Sort::firstPass( char *inFile, int bufferNum, int& tempHFnum )
{
	Status s;
	HeapFile inF(inFile, s);
	if (s != OK)
		return FAIL;
	Scan* scan = inF.openScan(s);
	int mSize = bufNum * PAGESIZE;
	char* memory = new char[mSize];
	char* res = new char[recSize];
	tempHFnum = 0;
	Status notFinish = OK;
	while (notFinish == OK)
	{
		int tot = 0;
		while ((tot + 1) * recSize <= mSize)
		{
			RID rid;
			int len;
			notFinish = scan->getNext(rid, res, len);
			if (notFinish != OK)
				break;
			memcpy(memory + tot * recSize, res, recSize);
			tot++;
		}
		if (tot > 0)
		{
			tempHFnum++;
			makeHFname(HFname, 1, tempHFnum);
			HeapFile p1f(HFname, s);
			qsort(memory, tot, recSize, sortCmp);
			for (int i = 0; i < tot; ++i)
			{
				memcpy(res, memory + i * recSize, recSize);
				RID rid;
				p1f.insertRecord(res, recSize, rid);
				//cout << res << " ";
			}
			//cout << endl;
		}
	}
	delete memory;
	delete res;
	return OK;
}

// pass after the first.
Status Sort::followingPass( int passNum, int oldHFnum,
						int bufferNum, int& newHFnum )
{
	HeapFile** fileArray = new HeapFile*[bufferNum];
	Scan** scan = new Scan*[bufferNum];
	int tot = 0;
	newHFnum = 0;
	Status s;
	for (int i = 1; i <= oldHFnum; i++)
	{
		makeHFname(HFname, passNum - 1, i);
		fileArray[tot] = new HeapFile(HFname, s);
		scan[tot] = fileArray[tot] ->openScan(s);
		tot++;
		if (tot == bufferNum || i == oldHFnum)
		{
			makeHFname(HFname, passNum, ++newHFnum);
			HeapFile hf(HFname, s);
			merge(scan, tot, &hf);
			for (int j = 0; j < tot; ++j)
				delete fileArray[j];
			tot = 0;
		}
	}
	for (int i = 1; i <= oldHFnum; i++)
	{
		makeHFname(HFname, passNum - 1, i);
		HeapFile file(HFname, s);
		file.deleteFile();
	}
	delete fileArray;
	delete scan;
	return OK;
}

// merge.
Status Sort::merge( Scan* scan[], int runNum, HeapFile* outHF )
{
	char** res = new char*[runNum];
	bool* ok = new bool[runNum];
	Status s;
	RID rid;
	for (int i = 0; i < runNum; ++i)
	{
		res[i] = new char[recSize];
		s = scan[i]->getNext(rid, res[i], recSize);
		if (s != OK)
			ok[i] = false;
		else
			ok[i] = true;
	}
	int i;
	while (true)
	{
		i = -1;
		for (int j = 0; j < runNum; ++j)
			if (ok[j] && (i == -1 || sortCmp(res[j], res[i]) == -1))
				i = j;
		if (i == -1)
			break;
		outHF->insertRecord(res[i], recSize, rid);
		s = scan[i]->getNext(rid, res[i], recSize);
		if (s != OK)
			ok[i] = false;
		else
			ok[i] = true;
	}
	for (int i = 0; i < runNum; ++i)
		delete res[i];
	delete res;
	delete ok;
	return OK;
}

