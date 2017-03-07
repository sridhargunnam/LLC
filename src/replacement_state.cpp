#include "replacement_state.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This file is distributed as part of the Cache Replacement Championship     //
// workshop held in conjunction with ISCA'2010.                               //
//                                                                            //
//                                                                            //
// Everyone is granted permission to copy, modify, and/or re-distribute       //
// this software.                                                             //
//                                                                            //
// Please contact Aamer Jaleel <ajaleel@gmail.com> should you have any        //
// questions                                                                  //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

/*
** This file implements the cache replacement state. Users can enhance the code
** below to develop their cache replacement ideas.
**
*/


////////////////////////////////////////////////////////////////////////////////
// The replacement state constructor:                                         //
// Inputs: number of sets, associativity, and replacement policy to use       //
// Outputs: None                                                              //
//                                                                            //
// DO NOT CHANGE THE CONSTRUCTOR PROTOTYPE                                    //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
CACHE_REPLACEMENT_STATE::CACHE_REPLACEMENT_STATE( UINT32 _sets, UINT32 _assoc, UINT32 _pol )
{

    numsets    = _sets;
    assoc      = _assoc;
    replPolicy = _pol;

    mytimer    = 0;

    InitReplacementState();
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function initializes the replacement policy hardware by creating      //
// storage for the replacement state on a per-line/per-cache basis.           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::InitReplacementState()
{
    // Create the state for sets, then create the state for the ways
    repl  = new LINE_REPLACEMENT_STATE* [ numsets ];
    PSEL=511 ; // PSEL is 10 bit where MSB is used to select policy for follower sets
    // ensure that we were able to create replacement state
    assert(repl);
    // Create the state for the sets
    for(UINT32 setIndex=0; setIndex<numsets; setIndex++) 
    {
        repl[ setIndex ]  = new LINE_REPLACEMENT_STATE[ assoc ];
	
        for(UINT32 way=0; way<assoc; way++) 
        {
            // initialize stack position (for true LRU)
            repl[ setIndex ][ way ].LRUstackposition = way;
	    repl[ setIndex ][ way ].RRVPstackposition = 3 ;  // this can be set to 2^m-1 as well  // make it 3

		// SHiP initilization
	    repl[ setIndex ][ way ].outcome = 0;
	    repl[ setIndex ][ way ].signature_m=0;

        }
    }

	for(UINT32 i=0; i<16384; i++)
	{	
	SHCT[i]=0;
	}	

    // Contestants:  ADD INITIALIZATION FOR YOUR HARDWARE HERE
    // PLRU initilization
//	plru_tree = { 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0 };
	plru_tree = new  UINT32*[numsets] ;
	assert(plru_tree);
	for(UINT32 setIndex=0; setIndex<numsets; setIndex++) 
	{
	plru_tree[setIndex] = new UINT32[15]	;
	for (UINT32 i = 0; i <= 14; i++) 
	{
		if( i==0 || i==2 || i==3 || i==4 || i==10 || i==12 || i==13)
        	plru_tree[setIndex][i] = 1;
		else
		plru_tree[setIndex][i]=0;
    	}
	}
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function is called by the cache on every cache miss. The input        //
// arguments are the thread id, set index, pointers to ways in current set    //
// and the associativity.  We are also providing the PC, physical address,    //
// and accesstype should you wish to use them at victim selection time.       //
// The return value is the physical way index for the line being replaced.    //
// Return -1 if you wish to bypass LLC.                                       //
//                                                                            //
// vicSet is the current set. You can access the contents of the set by       //
// indexing using the wayID which ranges from 0 to assoc-1 e.g. vicSet[0]     //
// is the first way and vicSet[4] is the 4th physical way of the cache.       //
// Elements of LINE_STATE are defined in crc_cache_defs.h                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::GetVictimInSet( UINT32 tid, UINT32 setIndex, const LINE_STATE *vicSet, UINT32 assoc,
                                               Addr_t PC, Addr_t paddr, UINT32 accessType )
{
    // If no invalid lines, then replace based on replacement policy
    if( replPolicy == CRC_REPL_LRU ) 
    {
        return Get_LRU_Victim( setIndex );
    }
    else if( replPolicy == CRC_REPL_RANDOM )
    {
        return Get_Random_Victim( setIndex );
    }
    else if( replPolicy == CRC_REPL_SRRIP )
    {
        // Contestants:  ADD YOUR VICTIM SELECTION FUNCTION HERE
        return Get_SRRIP_Victim( setIndex );
    }
    else if( replPolicy == CRC_REPL_BIP )
    {
        // BIP victim selection is same as LRU ; DRRIP victim selection is exactly same as SRRIP 
        return Get_LRU_Victim( setIndex );
    }
    else if( replPolicy == CRC_REPL_BRRIP )
    {
        return Get_SRRIP_Victim( setIndex );    // victim selection is same for SRRIP and BRRIP

    }
    else if( replPolicy == CRC_REPL_DRRIP )
    {
        return Get_SRRIP_Victim( setIndex );    // victim selection is same for SRRIP and BRRIP hence same for DRRIP
    }
    else if( replPolicy == CRC_REPL_SHIPPC )
    {
        return Get_SRRIP_Victim( setIndex );    // victim selection for ship-pc is same for SRRIP 
    }
    else if( replPolicy == CRC_REPL_PLRU )
    {
        return Get_PLRU_Victim( setIndex );    
    }


    // We should never get here
    assert(0);

    return -1; // Returning -1 bypasses the LLC
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function is called by the cache after every cache hit/miss            //
// The arguments are: the set index, the physical way of the cache,           //
// the pointer to the physical line (should contestants need access           //
// to information of the line filled or hit upon), the thread id              //
// of the request, the PC of the request, the accesstype, and finall          //
// whether the line was a cachehit or not (cacheHit=true implies hit)         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateReplacementState( 
    UINT32 setIndex, INT32 updateWayID, const LINE_STATE *currLine, 
    UINT32 tid, Addr_t PC, UINT32 accessType, bool cacheHit )
{
    // What replacement policy?
    if( replPolicy == CRC_REPL_LRU ) 
    {
        UpdateLRU( setIndex, updateWayID );
    }
    else if( replPolicy == CRC_REPL_RANDOM )
    {
        // Random replacement requires no replacement state update
    }
    else if( replPolicy == CRC_REPL_SRRIP )
    {	
        // Contestants:  ADD YOUR UPDATE REPLACEMENT STATE FUNCTION HERE
        // Feel free to use any of the input parameters to make
        // updates to your replacement policy
        // it takes cacheHit argument to update rrpv value to 0, on miss to (assoc -1) / 2^M-1
        UpdateSRRIP(setIndex, updateWayID, cacheHit);
    }
    else if( replPolicy == CRC_REPL_BIP )
    {	
        UpdateBIP(setIndex, updateWayID, cacheHit);
    }   
    else if( replPolicy == CRC_REPL_BRRIP )
    {	
        UpdateBRRIP(setIndex, updateWayID, cacheHit);
    }
    else if( replPolicy == CRC_REPL_DRRIP )
    {	
        UpdateDRRIP(setIndex, updateWayID, cacheHit);
    }
    else if( replPolicy == CRC_REPL_SHIPPC )
    {	
        UpdateSHIPPC(setIndex, updateWayID, cacheHit, PC);
    }
    else if( replPolicy == CRC_REPL_PLRU )
    {	
        UpdatePLRU(setIndex, updateWayID, cacheHit);
    }

     
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//////// HELPER FUNCTIONS FOR REPLACEMENT UPDATE AND VICTIM SELECTION //////////
//                                                                            //
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function finds the LRU victim in the cache set by returning the       //
// cache block at the bottom of the LRU stack. Top of LRU stack is '0'        //
// while bottom of LRU stack is 'assoc-1'                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_LRU_Victim( UINT32 setIndex )
{
    // Get pointer to replacement state of current set
    LINE_REPLACEMENT_STATE *replSet = repl[ setIndex ];

    INT32   lruWay   = 0;

    // Search for victim whose stack position is assoc-1
    for(UINT32 way=0; way<assoc; way++) 
    {
        if( replSet[way].LRUstackposition == (assoc-1) ) 
        {
            lruWay = way;
            break;
        }
    }

    // return lru way
    return lruWay;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function finds the SRRIP victim in the cache set		      //
// from left to right the first RRPV value with assoc-1 / 2^M-1 is returned	//
// //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_SRRIP_Victim( UINT32 setIndex )
{
    // Get pointer to replacement state of current set
    LINE_REPLACEMENT_STATE *replSet = repl[ setIndex ];

    INT32   srripWay   = -1;
    INT32   rrpvSaturatedFlag = 0 ;
    // Search for victim whose stack position is assoc-1
    for(UINT32 way=0; way<assoc; way++) 
    {
        if( replSet[way].RRVPstackposition ==  3) //assoc-1) ) 	// it could be 2^M-1
        {
            srripWay = way;
	    rrpvSaturatedFlag = 1 ;
            break;
        }
    }

    if( rrpvSaturatedFlag ==0 )
	{ 
 		for(UINT32 way=0; way<assoc; way++) 
    		{
        	replSet[way].RRVPstackposition++;	// increment every RRPV by 1 untill you make one of them assoc-1 or 2^M -1
    		}
	return Get_SRRIP_Victim( setIndex );
	}
		
    // return srrip way
    return srripWay;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function finds a random victim in the cache set                       //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_Random_Victim( UINT32 setIndex )
{
    INT32 way = (rand() % assoc);
    
    return way;
}

////////////////////////////////////////////////////////////////////////////////
//
//  PLRU victim selection
//////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_PLRU_Victim( UINT32 setIndex )
{

// this is the implementation of Figure 5 in PLRU paper
// traverse from root to leaf node based on plru bits 
INT32 way;
UINT32 i=0;
    while (i<15)
	{
	if (plru_tree[setIndex][i] == 1)
	{	
		way=i;
		i=(2*i)+2;
	}
	else
	{
		way=i;
		i=(2*i)+1;
	}
	}
    return way;
}

////////////////////////////////////////////////////////////////////////////////
//
//                                                                            //
// This function implements the LRU update routine for the traditional        //
// LRU replacement policy. The arguments to the function are the physical     //
// way and set index.                                                         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateLRU( UINT32 setIndex, INT32 updateWayID )
{
    // Determine current LRU stack position
    UINT32 currLRUstackposition = repl[ setIndex ][ updateWayID ].LRUstackposition;

    // Update the stack position of all lines before the current line
    // Update implies incremeting their stack positions by one
    for(UINT32 way=0; way<assoc; way++) 
    {
        if( repl[setIndex][way].LRUstackposition < currLRUstackposition ) 
        {
            repl[setIndex][way].LRUstackposition++;
        }
    }

    // Set the LRU stack position of new line to be zero
    repl[ setIndex ][ updateWayID ].LRUstackposition = 0;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function implements the SRRIP update routine.			      //
// takes index cachehit and update wayID 				      //
// In case of hit it makes the position as 0 and in case of miss it 	      //
// rrvp to assoc -1 or 2^M-1 						      //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateSRRIP( UINT32 setIndex, INT32 updateWayID, bool cacheHit )
{
        if( cacheHit ==1 ) 
        {
		 repl[ setIndex ][ updateWayID ].RRVPstackposition = 0;

        }
	else 
	{
		 repl[ setIndex ][ updateWayID ].RRVPstackposition = 2 ; //assoc - 1 ;    /// make it 2
	}
    

}
//
// is update policy for BRRIP same ?
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateBRRIP( UINT32 setIndex, INT32 updateWayID, bool cacheHit )
{

    bool  episilon = (rand() % 31 == 0); 
        if( cacheHit ==1 ) 
        {
		 repl[ setIndex ][ updateWayID ].RRVPstackposition = 0;

        }
	else 
	{
	if( episilon ==0 )
		 repl[ setIndex ][ updateWayID ].RRVPstackposition = 2 ; // assoc - 2 ;  /// 2
	else
		 repl[ setIndex ][ updateWayID ].RRVPstackposition = 3 ; //assoc - 1 ;  /// 3
	}

//if( episilon == 31)
//	epi_ctr = 0 ;
//else 
//	epi_ctr++ ;
//    

}
////////////////////////////////////////////////////////////////////////////////
// is update policy for DRRIP
// ////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateDRRIP( UINT32 setIndex, INT32 updateWayID, bool cacheHit)   // put PSEL here
{
UINT32 setIndex9to5 , setIndex4to0, InvsetIndex9to5 ;
////////// InvsetIndex9to5 selects the constituency and InvsetIndex9to5 selects the offset
setIndex9to5 = (setIndex & 992 ) ; // 1111100000 = 992 
setIndex4to0 = (setIndex & 31 ) ; // 0000011111 = 31 
InvsetIndex9to5 = (~setIndex9to5 & 1023)>>5 ;       // used to compare if 9to5 is compliment of 4to0 bits 
bool srrip_pol = (setIndex9to5>>5 == setIndex4to0);  // for SRRIP dedicated sets
bool brrip_pol = (InvsetIndex9to5 == setIndex4to0) ; // for BRRIP dedicated sets
bool psel_msb = ( PSEL > 511) ;
	if ( srrip_pol==0 && brrip_pol==0)
	{	// for follower sets
		if(!psel_msb)                                                   // not of psel
			UpdateSRRIP( setIndex, updateWayID, cacheHit);
		else 
			UpdateBRRIP( setIndex, updateWayID, cacheHit);
	}
		// for srrip sets
	else if (srrip_pol==1)
			{
			UpdateSRRIP( setIndex, updateWayID, cacheHit);
			if(PSEL<1023 && !cacheHit)
			PSEL++ ;
			}
		// for brrip sets
	else if (brrip_pol ==1)
			{
			UpdateBRRIP( setIndex, updateWayID, cacheHit);
			if(PSEL>0 && !cacheHit)
			PSEL--;
			}

}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// is update policy for DRRIP
// ////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateSHIPPC( UINT32 setIndex, INT32 updateWayID, bool cacheHit, Addr_t PC)  
{
	// hashing the PC address, right now taking only last 14 bits, have to do collision analysis if not doing hashing
	// take saturating counter of width 3 for shct counter 
	UINT32 SHCT_index= ((UINT32)PC) & 16383 ;		// gettin gthe signature from PC

///	implementation of figure 1 (b) SHiP algorithm
	if(cacheHit)
		{
		repl[ setIndex ][ updateWayID ].outcome = 1 ;
		if(SHCT[repl[ setIndex ][ updateWayID ].signature_m]<8)
		{
		SHCT[repl[ setIndex ][ updateWayID ].signature_m]++;
		}
		repl[ setIndex ][ updateWayID ].RRVPstackposition = 0 ;			// promotion
		}
	else 
		{
	        if(repl[ setIndex ][ updateWayID ].outcome==0)
			{
			if(SHCT[repl[ setIndex ][ updateWayID ].signature_m]>0)
			SHCT[repl[ setIndex ][ updateWayID ].signature_m]--;
			}
		repl[ setIndex ][ updateWayID ].signature_m = SHCT_index;
		repl[ setIndex ][ updateWayID ].outcome = 0;			
		if (SHCT[SHCT_index] == 0)
		{
			repl[ setIndex ][ updateWayID ].RRVPstackposition = 3 ; 	// distant reference	
		} 
		else
		{ 
			repl[ setIndex ][ updateWayID ].RRVPstackposition = 2 ;		// intermediate reference
		}
		}
		
}
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function implements the BIP update routine.			      //
// takes index cachehit and update wayID 				      //
// LIP : In case of rereference they are promoted from LRU to MRU position - Qureshi section 4. para2 //
// BIP : With low probability episilon insert at MRU position  - Qureshi section 4. para 3 //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateBIP( UINT32 setIndex, INT32 updateWayID, bool cacheHit )
{
 // Determine current LRU stack position
    UINT32 currLRUstackposition = repl[ setIndex ][ updateWayID ].LRUstackposition;
       
// epison is probability with which insertion takes place
    bool episilon ; episilon = (rand() % 31 == 0); 
	//
	// On cache hit means reference ; we just need to make lru stackposition vlaue 0 and change others accordingly 
	if( cacheHit ==1 )
        {
		for(UINT32 way=0; way<assoc; way++)
			{	if(repl[setIndex][way].LRUstackposition < currLRUstackposition)
				{
					repl[ setIndex ][ way ].LRUstackposition++;
				}
			}
    		
		repl[ setIndex ][ updateWayID ].LRUstackposition = 0;
	}

	else if(episilon ==1)  
	{	repl[setIndex][updateWayID].LRUstackposition = 0 ;
		for(UINT32 way=0; way<assoc; way++)
		{	if(int(way) != updateWayID)
			{	repl[setIndex][way].LRUstackposition++ ;
			}
		}
	}

	else 
		repl[setIndex][updateWayID].LRUstackposition = assoc -1 ; /// make it 2 bit ?	
		// this is by default, actually victim is selected from assoc -1 place,
		// so this is from assoc -1 place only
}    
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdatePLRU( UINT32 setIndex, INT32 updateWayID, bool cacheHit)
{
//// Used arrays to to implement the plru bits of each cache index
UINT32 node = updateWayID;  // fig 6 ;    while ( p is not the root) 
UINT32 plru_index ;
UINT32 left_child=-1 ; // 1 imples left child, 0 implies right child, -1 imples end node 
UINT32 parent_node_offset = 15 ; // for first iteration in while loop, sub 15 from  
//	if(node>7)
//	node=node-7;

	if(cacheHit)
	{
	node= updateWayID + parent_node_offset ;		// this the arithematic I developed by extending the plru tree so that it can be used to calculate the plru array index iteratively. The offset to be added to wayID to get the next left node of the extended tree is 15  
	while(node!=0)
	{
	if(node % 2 == 1)  // if p is a left child, (here result =1 imples left child) 
		{	
		left_child=1;
		}
	plru_index = (node-1)/2;	// finding the parent index
	if(left_child==1)
		{	
		plru_tree[setIndex][plru_index]=1;
		}
	else if(left_child==0)
		{
		plru_tree[setIndex][plru_index]=0;
		}
	node = plru_index ;			   
//	parent_node_offset = parent_node_offset>>1;
	}
	}
}
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// The function prints the statistics for the cache                           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
ostream & CACHE_REPLACEMENT_STATE::PrintStats(ostream &out)
{

    out<<"=========================================================="<<endl;
    out<<"=========== Replacement Policy Statistics ================"<<endl;
    out<<"=========================================================="<<endl;

    // CONTESTANTS:  Insert your statistics printing here

    return out;
    
}

