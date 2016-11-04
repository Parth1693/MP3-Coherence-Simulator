/*******************************************************
                          main.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#include "cache.h"
#include <string>
using namespace std;

int main(int argc, char *argv[])
{	
	ifstream traceFile;
	string fileName;
	char mystr[40];	
	char *p;
	string addressFull;
	ulong address;
	int i;

	p = &mystr[0];

	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <L1cache_size> <L2cache_size> <L1cache_assoc> <L2cache_assoc> <block_size> <trace_file> \n");
		 return 0;
        }

	/*****uncomment the next five lines*****/
	int L1cache_size = atoi(argv[1]);
	int L2cache_size = atoi(argv[2]);
	int L1cache_assoc= atoi(argv[3]);
	int L2cache_assoc= atoi(argv[4]);
	int blk_size   = atoi(argv[5]);
	fileName = argv[6];


	int num_processors = 4;  	
	
	//Print personal information at start of each run.
	cout <<"===== 506 Personal information =====" << '\n'
	<< "Parth Prashant Bhogate" << '\n'
	<< "UnityID: 200108628" << '\n'
	<< "SECTION ECE 506-001" << endl;

	//****************************************************//
	//*******print out simulator configuration here*******//
	//****************************************************//
	cout << "===== 506 SMP Simulator configuration =====" << '\n'
	<< "L1_SIZE:     \t" << L1cache_size << '\n'
	<< "L2_SIZE:     \t" << L2cache_size << '\n'
	<< "L1_ASSOC:    \t"<< L1cache_assoc << '\n'
	<< "L2_ASSOC:    \t"<< L2cache_assoc << '\n'
	<< "BLOCKSIZE:   \t" << blk_size << '\n'
	<< "TRACE FILE:  \t" << fileName << endl;
 
	//*********************************************//
        //*****create an array of caches here**********//
	//*********************************************//	

	//Instantiate L1 caches
	L1Cache **L1cachesArray = new L1Cache *[num_processors];

	for(i=0;i<num_processors;i++)
	{
		L1cachesArray[i] = new L1Cache(L1cache_size, L1cache_assoc, blk_size);
	}

	//Instantiate L2 caches
	L2Cache **L2cachesArray = new L2Cache *[num_processors];

	for(i=0;i<num_processors;i++)
	{
		L2cachesArray[i] = new L2Cache(L2cache_size, L2cache_assoc, blk_size);
	}

	//Open traceFile for reading
	strncpy(mystr, fileName.c_str(), fileName.length());
	mystr[fileName.length()]=0;

	traceFile.open(p);
	
	int proc = 0;
	int cnt = 0;
	string sub2;
	
	//Read processor number, r/w address from trace file
	while(traceFile>>proc>>sub2>>hex>>address)
	{
			
			cnt++;
			//proc = atoi(sub1.c_str());
			//cout << "Access no " << cnt << endl;
			//cout << "Proc no. is " << proc << " Op is " << sub2 << " Address is " << address << endl;

			//Send r/w address to a particular processor.
			int L1transaction = -1;
      int L2transaction = -1;
      int t = -1;
			int check = -1;

			L1transaction = L1cachesArray[proc]->L1Access( (ulong) address, sub2.c_str());

   			for(int i=0;i<num_processors;i++)
   			{            
     			if ( i == proc )
         			continue;

      			check = L2cachesArray[i]->L2searchCache( (ulong) address);
      			if ( check == 1 )
             		break;
   			}   

   			//L1 to L2 transactions
   			if ( L1transaction == readMiss )
   			{
          string op = "r";
   				L2transaction = L2cachesArray[proc]->L2Access((ulong) address, op.c_str(), check);
   			}

        else if ( L1transaction == readHit)
        {
           //Do nothing.
        }

   			else if ( L1transaction == writeHit )
   			{
          string op = "w";
   				L2transaction = L2cachesArray[proc]->L2Access((ulong) address, op.c_str(), check);
   			}

   			else if ( L1transaction == writeMiss )
   			{
          string op = "w";
   				L2transaction = L2cachesArray[proc]->L2Access((ulong) address, op.c_str(), check);
   			}

   			else if ( L1transaction == Done)
   			{
   				//Done.
          cout << "Error!" << endl;
   			}

   			//L2 cache coherence and back invalidation transactions
   			if ( L2transaction == BusRd)
   			{
   				for(int i=0;i<num_processors;i++)
            	{
               		if ( i == proc )
                  		continue;
               		string op = "d";
               		t = L2cachesArray[i]->L2Access( (ulong) address, op.c_str(), check);
                if(t != Done)
                  cout << "Error" << endl;                  
           		}
   			}

   			else if ( L2transaction == BusRdEvict)
   			{
   				for(int i=0;i<num_processors;i++)
            	{
               		if ( i == proc )
                  		continue;
               		string op = "d";
               		t = L2cachesArray[i]->L2Access( (ulong) address, op.c_str(), check);
                if(t != Done)
                  cout << "Error" << endl;                  
           		}
           		//Issue back invalidation
           		string op = "i";
           		t = L1cachesArray[proc]->L1Access( L2cachesArray[proc]->evictAddress, op.c_str() );
                if(t != Done)
                  cout << "Error" << endl;
   			}

   			else if ( L2transaction == BusRdX)
   			{
 
         		for(int i=0;i<num_processors;i++)
         		{
            		if ( i == proc )
               			continue;
            		string op = "x";
            		t = L2cachesArray[i]->L2Access( (ulong) address, op.c_str(), check);
                if(t != Done)
                  cout << "Error" << endl;                
         		}

            //Back Invalidate other L1 caches.
            for(int i=0;i<num_processors;i++)
            {
                if ( i == proc )
                    continue;
                string op = "i";
                t = L1cachesArray[i]->L1Access( (ulong) address, op.c_str());
                if(t != Done)
                  cout << "Error" << endl;                
            }            

   			}

   			else if ( L2transaction == BusRdXEvict)
   			{
   				//Back Invalidation to L1.
   				string op = "i";
   				t = L1cachesArray[proc]->L1Access( L2cachesArray[proc]->evictAddress, op.c_str() );
                if(t != Done)
                  cout << "Error" << endl;          

         		for(int i=0;i<num_processors;i++)
         		{
            		if ( i == proc )
               			continue;
            		string op = "x";
            		t = L2cachesArray[i]->L2Access( (ulong) address, op.c_str(), check);
                if(t != Done)
                  cout << "Error" << endl;                
         		}

            //Back Invalidate other L1 caches.
            for(int i=0;i<num_processors;i++)
            {
                if ( i == proc )
                    continue;
                string op = "i";
                t = L1cachesArray[i]->L1Access( (ulong) address, op.c_str());
                if(t != Done)
                  cout << "Error" << endl;                
            }    

   			}

   			else if ( L2transaction == BusUpgr)
   			{
            	for(int i=0;i<num_processors;i++)
            	{
               		if( i == proc )     /* OTHER caches only */
               			continue;
               		string op = "u";
               		t = L2cachesArray[i]->L2Access( (ulong) address, op.c_str(), check);
                if(t != Done)
                  cout << "Error" << endl;                  
            	}

            //Back Invalidate other L1 caches.
            for(int i=0;i<num_processors;i++)
            {
                if ( i == proc )
                    continue;
                string op = "i";
                t = L1cachesArray[i]->L1Access( (ulong) address, op.c_str());

                if(t != Done)
                  cout << "Error" << endl;
            }                  
   			}

   			else if ( L2transaction == Done)
   			{
   				//Done.
   			}



	} 
	
	traceFile.close();
		
	//********************************//
	//print out all caches' statistics //
	//********************************//
	
	for(i=0;i<num_processors;i++)
	{	
		cout << "============ Simulation results L1 Cache (Processor " << i << ")" << " ============" <<  endl;
      L1cachesArray[i]->L1printStats();

    cout << "============ Simulation results L2 Cache (Processor " << i << ")" << " ============" <<  endl;

    //Calculate memTransactions
    ulong m;

    m = L2cachesArray[i]->readMisses + L2cachesArray[i]->writeMisses + L2cachesArray[i]->writeBacks - L2cachesArray[i]->cache2cache;
      L2cachesArray[i]->L2printStats(m);

	}

}