#ifndef REPL_STATE_H
#define REPL_STATE_H

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

#include <cstdlib>
#include <cassert>
#include "utils.h"
#include "crc_cache_defs.h"

// Replacement Policies Supported
typedef enum 
{
    CRC_REPL_LRU        = 0,
    CRC_REPL_RANDOM     = 1,
    CRC_REPL_SRRIP = 2,
    CRC_REPL_BIP = 3,
    CRC_REPL_DIP = 4,
    CRC_REPL_BRRIP = 5,
    CRC_REPL_DRRIP = 6,
    CRC_REPL_SHIPPC = 7,
    CRC_REPL_PLRU = 8
} ReplacemntPolicy;

// Replacement State Per Cache Line
typedef struct
{
    UINT32  LRUstackposition;
    UINT32  RRVPstackposition;		// used in SRRIP, BRRIP and DRRIP
// For SHiP - outcome and signature_m 
    bool  outcome;
    UINT32 signature_m ;
    // CONTESTANTS: Add extra state per cache line here

} LINE_REPLACEMENT_STATE;
//// node for tree based PLRU 
//struct node
//{
//    UINT32 plrubit;
//    node* left;
//    node* right;
//};
//
//Node* newNode(int data)
//{
//    Node* node = new Node;
//    node->data = data;
//    node->left = node->right = NULL;
//    return node;
//}
///////////////////////////////////////
// The implementation for the cache replacement policy
class CACHE_REPLACEMENT_STATE
{

  private:
    UINT32 numsets;
    UINT32 assoc;
    UINT32 replPolicy;
    UINT32 SHCT[16384];				// For SHCT table 
    LINE_REPLACEMENT_STATE   **repl;
	UINT32 PSEL ;				// for set-dueling in DRRIP
	UINT32 **plru_tree ;			// pointer for plru array
    COUNTER mytimer;  // tracks # of references to the cache
    // CONTESTANTS:  Add extra state for cache here

  public:

    // The constructor CAN NOT be changed
    CACHE_REPLACEMENT_STATE( UINT32 _sets, UINT32 _assoc, UINT32 _pol) ; //, UINT32 _PSEL );

    INT32  GetVictimInSet( UINT32 tid, UINT32 setIndex, const LINE_STATE *vicSet, UINT32 assoc, Addr_t PC, Addr_t paddr, UINT32 accessType );
    void   UpdateReplacementState( UINT32 setIndex, INT32 updateWayID );

    void   SetReplacementPolicy( UINT32 _pol ) { replPolicy = _pol; } 
    void   IncrementTimer() { mytimer++; } 

    void   UpdateReplacementState( UINT32 setIndex, INT32 updateWayID, const LINE_STATE *currLine, 
                                   UINT32 tid, Addr_t PC, UINT32 accessType, bool cacheHit);

    ostream&   PrintStats( ostream &out);

  private:
    
    void   InitReplacementState();
    INT32  Get_Random_Victim( UINT32 setIndex );

    INT32  Get_LRU_Victim( UINT32 setIndex );
    void   UpdateLRU( UINT32 setIndex, INT32 updateWayID );
    
    INT32  Get_SRRIP_Victim( UINT32 setIndex );
    void   UpdateSRRIP( UINT32 setIndex, INT32 updateWayID, bool cacheHit );

    void   UpdateBIP( UINT32 setIndex, INT32 updateWayID, bool cacheHit );

 //   void   Get_BRRIP_Victim( UINT32 setIndex ); BRRIP, SHIP PC victim selection is same as SRRIP
    void   UpdateBRRIP( UINT32 setIndex, INT32 updateWayID, bool cacheHit );
    void   UpdateDRRIP( UINT32 setIndex, INT32 updateWayID, bool cacheHit );
// for SHiP
    void   UpdateSHIPPC( UINT32 setIndex, INT32 updateWayID, bool cacheHit, Addr_t PC) ;
// plru
	INT32  Get_PLRU_Victim( UINT32 setIndex );
	void   UpdatePLRU( UINT32 setIndex, INT32 updateWayID, bool cacheHit );


};


#endif
