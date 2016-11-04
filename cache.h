/*******************************************************
                          cache.h
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <math.h>
#include <iomanip>
#include <stdlib.h>
#include <assert.h>
#include <iomanip>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum{
	INVALID = 0,
  VALID,
	SHARED,
	MODIFIED,
  EXCLUSIVE
};

//Return values enum
enum{
  readMiss = 1,
  readHit,
  writeHit,
  writeMiss,
  BusRd,
  BusRdX,
  BusUpgr,
  BusRdXEvict,
  BusRdEvict,
  Done
};

class cacheLine 
{
public:
   ulong tag;
   ulong Flags;   // 0:INVALID, 1:SHARED, 2:MODIFIED, 3:EXCLUSIVE 
   ulong seq; 
 
//public:
   cacheLine()            { tag = 0; Flags = 0; }
   ulong getTag()         { return tag; }
   ulong getFlags()			{ return Flags;}
   ulong getSeq()         { return seq; }
   void setSeq(ulong Seq)			{ seq = Seq;}
   void setFlags(ulong flags)			{  Flags = flags;}
   void setTag(ulong a)   { tag = a; }
   void invalidate()      { tag = 0; Flags = INVALID; }         //useful function
   bool isValid()         { return ((Flags) != INVALID); }
};

class L1Cache
{
  public:
    ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
    ulong reads,readMisses, writes, writeMisses, cacheFills, missRate, backInvalidations; 
    ulong evictions;

   cacheLine **cache;           //For cache blocks
   ulong L1calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong L1calcIndex(ulong addr)  { return ((addr >> log2Blk) & tagMask);}
   ulong L1calcAddr4Tag(ulong tag)   { return (tag << (log2Blk));} 

   ulong currentCycle; 

    L1Cache(int,int,int);
   ~L1Cache() { delete cache;}
   
   cacheLine *L1findLineToReplace(ulong addr);
   cacheLine *L1fillLine(ulong addr);
   cacheLine * L1findLine(ulong addr);
   cacheLine * L1getLRU(ulong);
   void L1updateLRU(cacheLine *);
   
   ulong L1getRM(){return readMisses;} ulong L1getWM(){return writeMisses;} 
   ulong L1getReads(){return reads;}ulong L1getWrites(){return writes;}
   void L1printStats();

   int L1Access(ulong, const char*);
};

class L2Cache
{
public:
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses, writes, writeMisses;
   ulong writeBacks, cache2cache, interventions, invalidations, flushes, evictions;
   ulong evictAddress;

   cacheLine **cache;           //For cache blocks
   ulong L2calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong L2calcIndex(ulong addr)  { return ((addr >> log2Blk) & tagMask);}
   ulong L2calcAddr4Tag(ulong tag)   { return (tag << (log2Blk));}
   
   ulong currentCycle;    
     
    L2Cache(int,int,int);
   ~L2Cache() { delete cache;}
   
   cacheLine *L2findLineToReplace(ulong addr);
   cacheLine *L2fillLine(ulong addr);
   cacheLine * L2findLine(ulong addr);
   cacheLine * L2getLRU(ulong);
   
   ulong L2getRM(){return readMisses;} ulong L2getWM(){return writeMisses;} 
   ulong L2getReads(){return reads;}ulong L2getWrites(){return writes;}
   
   int L2Access(ulong, const char*, int);
   int evict;

   void L2writeBack(ulong)   {writeBacks++;}
   void L2printStats(ulong);
   void L2updateLRU(cacheLine *);
   int L2searchCache(ulong);
};



#endif
