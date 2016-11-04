/*******************************************************
                          cache.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#include "cache.h"
using namespace std;

//Constructor
L1Cache::L1Cache(int s,int a,int b)
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = backInvalidations = 0;
   
   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));    
   
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
	   tagMask <<= 1;
      tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
	     cache[i][j].invalidate();   //Make all lines invalid initially.
      }
   }      
   
}

//Constructor
L2Cache::L2Cache(int s,int a,int b)
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = 0;
   evict = 0;
   
   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));    
   
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
      tagMask <<= 1;
      tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
        cache[i][j].invalidate();   //Make all lines invalid initially.
      }
   }      
   
}

int L1Cache::L1Access(ulong addr, const char* op)
{
   currentCycle++;     /*per cache global counter to maintain LRU order 
                        among cache ways, updated on every cache access*/
   
   if ( *op == 'w')  writes++;
   else if ( *op == 'r') reads++;

   //Read request processing
   if ( *op == 'r')
   {
      cacheLine * line = L1findLine(addr);

      //Read Miss
      if (line == NULL)
      {
         readMisses++;

         //Bring in the block.
         //newLine is the MRU now.
         cacheLine *newline = L1fillLine(addr); 
         newline -> setFlags(VALID);

         //Send request to L2.
         return readMiss;  
      }

      //Read Hit
      else
      {
         L1updateLRU(line);
         line -> setFlags(VALID);
         return readHit;
      }

   } //Read request ends.

   //Write request processing.
   if ( *op == 'w')
   {
      cacheLine * line = L1findLine(addr);

      //Write Miss
      if(line == NULL)
      {
         writeMisses++;

         //Send write to L2 only. WT.
         return writeMiss;
      }

      //Write Hit
      else
      {
         L1updateLRU(line);
         //Bring in the block. 
         line->setFlags(VALID);

         return writeHit; 
      }

   } //Write request ends.

   //Back invalidation
   if ( *op == 'i')
   {
      cacheLine * line = L1findLine(addr);
      //backInvalidations++;

      //Line is not present in the cache.
      if (line == NULL)
      {  
         //Do nothing.
         return Done;
      }

      else
      {
         line->setFlags(INVALID);
         backInvalidations++;
         return Done;
      }
   }//Invalidation ends.

   return Done;
}

//******************* Access function for MESI protocol *****************************//

int L2Cache::L2Access(ulong addr, const char* op, int check)
{         
   currentCycle++;     /*per cache global counter to maintain LRU order 
                        among cache ways, updated on every cache access*/
         
   if( *op == 'w') writes++;
   else if ( *op == 'r' ) reads++;

if ( *op == 'l') //Update LRU only for Read Hit in L1
{
  reads++; 
  cacheLine * line = L2findLine(addr);
   if(line == NULL)
   {
      cout << "Error! Inclusion Violated!" <<  endl;
      return Done;
   }
   else
   {
      L2updateLRU(line);
      return Done;
   }
} 

/*** L1 requests ***/
if ( (*op == 'r') || (*op == 'w') )   
{   
   cacheLine * line = L2findLine(addr);

   if(line == NULL)  /*miss, meaning it is in INVALID state */
   {
      if( *op == 'w' ) writeMisses++;
      else readMisses++;

      cacheLine *newline = L2fillLine(addr);

      //newLine is the MRU now.

      if ( *op == 'w')
      {
         newline->setFlags(MODIFIED);

         if (check == 1)
            cache2cache++;

         //Issue BusRdX
         if ( evict == 1)
         return BusRdXEvict;
         else return BusRdX;

      }  //PrWr end.

      if ( *op == 'r' )
      {      
         //Check = 1 inplies some other cache also has the required block.
         //If check = 1, move to SHARED state.
         //If check = 0, move to EXCLUSIVE state.

         if ( check == 1 )
         {
            newline->setFlags(SHARED);
            cache2cache++;
      
            //Issue BusRd.
            if ( evict == 1)
            return BusRdEvict;
            else return BusRd;
         }

         else if ( check == 0 )
         {
            newline->setFlags(EXCLUSIVE);

            //Issue BusRd.
            if ( evict == 1)
            return BusRdEvict;
            else return BusRd;
         }

      }  //PrRd end.
      
   }  //MISS end

   else     /*hit*/
   {
      L2updateLRU(line);
      
      //MODIFIED state
      if( line->getFlags() == MODIFIED )
      {
         line->setFlags(MODIFIED);   //Stay in modified.

         return Done;
      }  //MODIFIED end

      //SHARED state
      else if( line->getFlags() == SHARED )
      {
         if( *op == 'r')
         {
            line->setFlags(SHARED);

            return Done;
         }

         if( *op == 'w')
         {
            line->setFlags(MODIFIED);

            //Issue BusUpgrade
            return BusUpgr;
         }

      }  //SHARED end

      //EXCLUSIVE state
      else if ( line->getFlags() == EXCLUSIVE )
      {
         if ( *op == 'r')
         {
            line->setFlags(EXCLUSIVE);

            return Done;
         }

         if ( *op == 'w')
         {
            line->setFlags(MODIFIED);

            return Done;
         }
      }  //EXCLUSIVE end.

   }  //HIT end
} //Processor request ends.


/*** Bus transactions ***/
//BusRd, BusRdX, BusUpgr and Back Invalidations

else if ( (*op == 'd') || (*op == 'x') || (*op == 'u'))
{
   cacheLine * line = L2findLine(addr);

   if(line == NULL)  /*miss, means it is INVALID state */
   {

      if ( *op == 'd' || *op == 'x' || *op == 'u' )
      {         
         //do nothing;
         return Done;
      }

   } //Miss ends

   else    /** Hit **/
   {
      //MODIFIED state
      if ( line->getFlags() == MODIFIED )
      {
         if ( *op == 'd')
         {
            line->setFlags(SHARED);  
            interventions++;    
   
            //Issue Flush *******
            L2writeBack(addr); 
            flushes++; 

            return Done;
         }

         if( *op == 'x' )
         {
            line->setFlags(INVALID);
            invalidations++;

            //Issue Flush ******
            L2writeBack(addr);
            flushes++;

            return Done;
         }

         if ( *op == 'u')
         {
            return Done;
         }
      } //MODIFIED end.

      //SHARED state
      else if ( line->getFlags() == SHARED )
      {
         if ( *op == 'd')
         {
            line->setFlags(SHARED);
            return Done;
            //Issue FlushOpt
         }

         if ( *op == 'x')
         {
            line->setFlags(INVALID);
            invalidations++;
            return Done;
            //Issue FlushOpt
         }

         if ( *op == 'u')
         {
            line->setFlags(INVALID);
            invalidations++;
            return Done;
         }
      } //SHARED end.

      //EXCLUSIVE state
      else if ( line->getFlags() == EXCLUSIVE )
      {
         if ( *op == 'd')
         {
            line->setFlags(SHARED);
            interventions++;
            
            return Done;
            //Issue FlushOpt
         }

         if ( *op == 'x' )
         {
            line->setFlags(INVALID);
            invalidations++;
            
            return Done;
            //Issue FlushOpt
         }

         if ( *op == 'u')
         {
            return Done;
         }
      } //EXCLUSIVE end.

   }  //Hit ends
      
} //Bus transaction ends

return Done;

}  /// L2Access ends.

/* Search cache to find whether it has a particular cache block.
This function is used to assert line 'C' if any cache copies of a block exist.
*/

int L2Cache::L2searchCache(ulong address)
{
   int found = 0;
   ulong tag, i;

   tag = L2calcTag(address);
   i   = L2calcIndex(address);

  for(ulong j=0; j<assoc; j++)
   if(cache[i][j].isValid())   //Either SHARED or MODIFIED or EXCLUSIVE
     if(cache[i][j].getTag() == tag)
      {
           found = 1; break; 
      }

   return found;
}

/*look up line*/
cacheLine * L1Cache::L1findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = L1calcTag(addr);
   i   = L1calcIndex(addr);
  
   for(j=0; j<assoc; j++)
   if(cache[i][j].isValid())   //Either SHARED or MODIFIED
     if(cache[i][j].getTag() == tag)
      {
           pos = j; break; 
      }
   if(pos == assoc)
   return NULL;
   else
   return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void L1Cache::L1updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if it exists, otherwise return LRU line*/
cacheLine * L1Cache::L1getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = L1calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   

   //Need to evict.
   evictions++;
   for(j=0;j<assoc;j++)
   {
    if(cache[i][j].getSeq() <= min) 
    { 
      victim = j; min = cache[i][j].getSeq();
    }
   } 
   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *L1Cache::L1findLineToReplace(ulong addr)
{
   cacheLine * victim = L1getLRU(addr);
   L1updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *L1Cache::L1fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = L1findLineToReplace(addr);
   assert(victim != 0);
      
   tag = L1calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);   

   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}


/*look up line*/
cacheLine * L2Cache::L2findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = L2calcTag(addr);
   i   = L2calcIndex(addr);
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())   //Either SHARED or MODIFIED
	  if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void L2Cache::L2updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if it exists, otherwise return LRU line*/
cacheLine * L2Cache::L2getLRU(ulong addr)
{
   ulong i, j, k, victim, min;
   evict = 0;

   victim = assoc;
   min    = currentCycle;
   i      = L2calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   

   //Need to evict. So send Back Invalidation to L1.
   evictions++;
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) 
    { 
      victim = j; min = cache[i][j].getSeq();
    }
   } 
   assert(victim != assoc);
   evict = 1;
   k = cache[i][victim].getTag();
   evictAddress = (k << log2Blk);

   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *L2Cache::L2findLineToReplace(ulong addr)
{
   cacheLine * victim = L2getLRU(addr);
   L2updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *L2Cache::L2fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = L2findLineToReplace(addr);
   if ( (victim->getFlags() == MODIFIED) )
      L2writeBack(addr);
   assert(victim != 0);
    	
   tag = L2calcTag(addr);   
   victim->setTag(tag);   

   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void L1Cache::L1printStats()
{ 
   float miss_rate = (float) (100 * ((float)(readMisses + writeMisses)/(float)(reads+writes)));
   streamsize default_prec = cout.precision(5);
   
   cout 
   << "01. number of reads:                  \t" << reads << '\n'
   << "02. number of read misses:            \t" << readMisses << '\n'
   << "03. number of writes:                 \t" << writes << '\n'   
   << "04. number of write misses:           \t" << writeMisses << endl;
   cout.precision(2);
   cout.setf(ios::fixed, ios::floatfield);
   cout << "05. total miss rate:                   \t" << miss_rate << "%" << endl;
   cout.unsetf(ios::floatfield);
   cout.precision(default_prec);
   cout << "06. number of back invalidations:\t" << backInvalidations << '\n' 
   << "07. number of fills:                  \t" << readMisses << '\n'
   << "08. number of evictions:              \t" << evictions << endl; 
}

void L2Cache::L2printStats(ulong m)
{ 
   float miss_rate = (float) (100 * ((float)(readMisses + writeMisses)/(float)(reads+writes)));
   streamsize default_prec = cout.precision(5);
   
   cout 
   << "01. number of reads:                  \t" << reads << '\n'
   << "02. number of read misses:            \t" << readMisses << '\n'
   << "03. number of writes:                 \t" << writes << '\n'   
   << "04. number of write misses:           \t" << writeMisses << endl;
   cout.precision(2);
   cout.setf(ios::fixed, ios:: floatfield);
   cout << "05. total miss rate:                   \t" <<  miss_rate << "%" << endl;
   cout.unsetf(ios::floatfield);
   cout.precision(default_prec);
   cout << "06. number of writebacks:            \t" << writeBacks << '\n'
   << "07. number of cache-to-cache transfers:\t" <<  cache2cache << '\n'
   << "08. number of memory transactions:    \t" << m << '\n'
   << "09. number of interventions:          \t" << interventions << '\n' 
   << "10. number of invalidations:          \t" << invalidations << '\n'
   << "11. number of flushes:                \t" << flushes << '\n'
   << "12. number of evictions:              \t" <<  evictions << endl;    
}


