#include "memory.h"
#include "math.h"

unsigned int clockX;
unsigned int numMisses;
int cache_org;

int tag;
int cIndex;
int offset;
//DIRECT mask values
int directTagMask = 0xFFFFFFe0; //27 bit tag mask
int directIndexMask = 0x0000001c; //3 bit mask for index
int wordOffsetMask = 0x00000003; //2 bits for byte offset within block
//TWOWAY mask values
int setTagMask = 0xFFFFFFF0; //28 bit tag mask
int setIndexMask = 0x0000000c; //2 bit mask for TWOWAY index
//int twowayOffsetMask = 0x00000003; //2 bits for byte offset within block


void resetClock()
{
  clockX = 0;
  numMisses = 0;
  for(int j=0; j<BLOCKS_IN_CACHE; j++) 
	{
		m.myCache.cblocks[j].tag = 0;
		m.myCache.cblocks[j].valid = 0;
		m.myCache.cblocks[j].last_used = 0;
	}

}

void printCacheOrg(int org)
{
  printf("Cache Organization: ");
  if (org == DIRECT)
    printf ("DIRECT MAPPED\n");
  else if (org == TWOWAY)
    printf ("TWO-WAY SET ASSOCIATIVE\n");
  else if (org == FULLY)
    printf ("FULLY ASSOCIATIVE\n");
  else
    printf ("ERROR: WRONG CACHE ORG\n");
}

// show cache contents: DIRECT or FULLY
void showCacheAddress()
{
	for(int j=0; j<BLOCKS_IN_CACHE; j++) 
	{
		printf("Address in block %d: ", j);
		for(int k=0; k<WORDS_PER_BLOCK; k++) 
		{
			// print out addresses of each block
			//if(cache_org == DIRECT)
				printf("%d  ",(m.myCache.cblocks[j].tag*32)+(j*4)+k);
			//else if(cache_org == TWOWAY)
			//	printf("%d  ",(m.myCache.cblocks[j].tag*16)+(j*4)+k);
			//else if(cache_org == FULLY)
			//	printf("%d  ",(m.myCache.cblocks[j].tag*4)+k);
		}
		printf("\n");
	}

    printf("\n");
}

int getData (int address)                // load
{
	int data;
	int wordIndex = address%WORDS_PER_BLOCK; //word index, 2 bits
	int addr = address/WORDS_PER_BLOCK; //remaining address to be used for tag and index, 30 bits
	
	// implement here
//*************START DIRECT Mapped***************
	if(cache_org == DIRECT)
	{	
		int cIndex = addr%BLOCKS_IN_CACHE; // direct cache index, 3 bits
		int tag = addr/BLOCKS_IN_CACHE;    // cache tag, 27 bits
  
		if(m.myCache.cblocks[cIndex].valid == 1 && m.myCache.cblocks[cIndex].tag == tag)
		{
			// cache hit, load data from cache
			data = m.myCache.cblocks[cIndex].data[wordIndex];
			clockX = clockX + 2;
		}
		else
		{
			// cache miss, load data from main memory into cache
			numMisses = numMisses + 1;
			clockX = clockX + 100;
			m.myCache.cblocks[cIndex] = mm.blocks[addr];
			m.myCache.cblocks[cIndex].tag = tag;
			data = m.myCache.cblocks[cIndex].data[wordIndex];
			m.myCache.cblocks[cIndex].valid = 1;
		}
	}
//*************END DIRECT Mapped**********************

//*************START TWOWAY set associative****************
	if(cache_org == TWOWAY)
	{
		int hit = 0;
		int sIndex = addr%(BLOCKS_IN_CACHE/BLOCKS_IN_SET); // set block index, 2 bits
		int tag = addr/(BLOCKS_IN_CACHE/BLOCKS_IN_SET); // two way tag, 28 bits
		
		int j;
		for(j=0; j<BLOCKS_IN_SET; j++)
		{
			if(m.myCache.cblocks[BLOCKS_IN_SET*sIndex+j].tag == tag && m.myCache.cblocks[BLOCKS_IN_SET*sIndex+j].valid == 1)
			//if(m.setCache[sIndex].setblocks[j].valid == 1 && m.setCache[sIndex].setblocks[j].tag == tag)
			{
				hit = 1;
				break;
			}
		}
		if(hit)
		{
			//cache hit, load data from cache
			clockX = clockX + 2;
			data = m.myCache.cblocks[BLOCKS_IN_SET*sIndex+j].data[wordIndex];
			m.myCache.cblocks[BLOCKS_IN_SET*sIndex+j].last_used = 1;
			m.myCache.cblocks[BLOCKS_IN_SET*sIndex+(j+1)%2].last_used = 0;
			//data = m.setCache[sIndex].setblocks[j].data[wordIndex];
		}
		else
		{
			// cache miss, load data from main memory into cache
			numMisses = numMisses + 1;
			for(int k = 0; k<BLOCKS_IN_SET; k++)
			{
				if(m.myCache.cblocks[BLOCKS_IN_SET*sIndex+k].last_used == 0)
				//if(m.setCache[sIndex].setblocks[k].last_used == 0)
				{
					m.myCache.cblocks[BLOCKS_IN_SET*sIndex+k] = mm.blocks[addr];
					//m.setCache[sIndex].setblocks[k] = mm.blocks[addr];
				
				m.myCache.cblocks[BLOCKS_IN_SET*sIndex+k].last_used = 1;
				m.myCache.cblocks[BLOCKS_IN_SET*sIndex+(k+1)%2].last_used = 0;
				m.myCache.cblocks[BLOCKS_IN_SET*sIndex+k].tag = tag;
				m.myCache.cblocks[BLOCKS_IN_SET*sIndex+k].valid = 1;
				/*m.setCache[sIndex].setblocks[k].last_used = 1;
				m.setCache[sIndex].setblocks[(k+1)%2].last_used = 0;
				m.setCache[sIndex].setblocks[k].tag = tag;
				m.setCache[sIndex].setblocks[k].valid = 1;*/
				break;
				}
			}
			data = mm.blocks[addr].data[wordIndex];
		}
	}
//***************END TWOWAY set associative******************

//***************START FULLY associative****************
	if(cache_org == FULLY)
	{
		int hit = 0;
		
		int i;
		for(i=0; i<BLOCKS_IN_CACHE; i++)
		{
			if(m.myCache.cblocks[i].tag == addr && m.myCache.cblocks[i].valid == 1)
			{
				printf("FULL Hit, get");
				hit = 1;
				break;
			}
			
		}
		if(hit)
		{
			for(int j=0; j<BLOCKS_IN_CACHE; j++) // reset cache last used values
				m.myCache.cblocks[j].last_used = 0;
			clockX = clockX + 2;
			data = m.myCache.cblocks[i].data[wordIndex];
			m.myCache.cblocks[i].last_used = 1; // set last used of cache block that was hit
		}
		else
		{
			//printf("FULL Misses in get");
			numMisses = numMisses + 1;
			for(int j = 0; j<BLOCKS_IN_CACHE; j++)
			{
				if(m.myCache.cblocks[j].last_used == 0)
				{
					//printf("FULL Hit, get");
					m.myCache.cblocks[j] = mm.blocks[addr];
				}
				m.myCache.cblocks[j].last_used = 1;
				m.myCache.cblocks[j].tag = addr;
				m.myCache.cblocks[j].valid = 1;
				break;
			}
			clockX = clockX + 100;
			data = mm.blocks[addr].data[wordIndex];
		}
	}
//***************END FULLY associative****************
  return data;
}

void putData (int address, int value)     // store
{
  // implement here
  int wordIndex = address%WORDS_PER_BLOCK;
  int addr = address/WORDS_PER_BLOCK;
//*************START DIRECT Mapped*************** 
 if(cache_org == DIRECT)
 {
	int cIndex = addr%BLOCKS_IN_CACHE;
	int tag = addr/BLOCKS_IN_CACHE;
	
  	//cIndex = (address & directIndexMask) >> 2;
	//tag = (address & directTagMask) >> 5;
	//offset = (address & directOffsetMask);
  
  if(m.myCache.cblocks[cIndex].valid == 1 && m.myCache.cblocks[cIndex].tag == tag)
  {
	  //cache hit, write to cache block and MM block
	  m.myCache.cblocks[cIndex].data[wordIndex] = value;
	  clockX = clockX + 2;
	  mm.blocks[addr].data[wordIndex] = value;
	  clockX = clockX + 100;
  }
  else
  {
	//cache miss, load from mem, write to cache, write to mem
	numMisses = numMisses + 1;
	clockX = clockX + 100;
	mm.blocks[addr].data[wordIndex] = value;
	clockX = clockX + 2;
	m.myCache.cblocks[cIndex] = mm.blocks[addr];
	m.myCache.cblocks[cIndex].tag = tag;
	m.myCache.cblocks[cIndex].valid = 1;
		  
  }
 }
//*************END DIRECT Mapped**********************

//*************START TWOWAY set associative****************
	if(cache_org == TWOWAY)
	{
		int hit = 0;
		int sIndex = addr%(BLOCKS_IN_CACHE/BLOCKS_IN_SET);
		int tag = addr/(BLOCKS_IN_CACHE/BLOCKS_IN_SET);
		
		int j;
		for(j=0; j<BLOCKS_IN_SET; j++)
		{
			if(m.myCache.cblocks[BLOCKS_IN_SET*sIndex+j].tag == tag && m.myCache.cblocks[BLOCKS_IN_SET*sIndex+j].valid == 1)
			//if(m.setCache[sIndex].setblocks[j].valid == 1 && m.setCache[sIndex].setblocks[j].tag == tag)
			{
				hit = 1;
				break;
			}
		}
		if(hit)
		{
			clockX = clockX + 2;
			m.myCache.cblocks[BLOCKS_IN_SET*sIndex+j].data[wordIndex] = value;
			m.myCache.cblocks[BLOCKS_IN_SET*sIndex+j].last_used = 1;
			m.myCache.cblocks[BLOCKS_IN_SET*sIndex+(j+1)%2].last_used = 0;
			//m.setCache[sIndex].setblocks[j].data[wordIndex] = value;
			clockX = clockX + 100;
			mm.blocks[addr].data[wordIndex] = value;
		}
		else
		{
			numMisses = numMisses + 1;
			clockX = clockX + 100;
			mm.blocks[addr].data[wordIndex] = value;
			clockX = clockX + 2;
			m.myCache.cblocks[BLOCKS_IN_SET*sIndex+j] = mm.blocks[addr];
			m.myCache.cblocks[BLOCKS_IN_SET*sIndex+j].last_used = 1;
			m.myCache.cblocks[BLOCKS_IN_SET*sIndex+(j+1)%2].last_used = 0;
			m.myCache.cblocks[BLOCKS_IN_SET*sIndex+j].tag = tag;
			m.myCache.cblocks[BLOCKS_IN_SET*sIndex+j].valid = 1;
			/*m.setCache[sIndex].setblocks[j] = mm.blocks[addr];
			m.setCache[sIndex].setblocks[j].last_used = 1;
			m.setCache[sIndex].setblocks[(j+1)%2].last_used = 0;
			m.setCache[sIndex].setblocks[j].tag = tag;
			m.setCache[sIndex].setblocks[j].valid = 1;*/
		}

	}
//***************END TWOWAY set associative******************

//***************START FULLY associative****************
	if(cache_org == FULLY)
	{
		int hit = 0;
		
		int i;
		for(i = 0; i<BLOCKS_IN_CACHE; i++)
		{
			if(m.myCache.cblocks[i].tag == addr && m.myCache.cblocks[i].valid == 1)
			{
				printf("********FULL Hit********");
				hit = 1;
				break;
			}
		}
		if(hit)
		{
			for(int j=0; j<BLOCKS_IN_CACHE; j++) // reset cache last used values
				m.myCache.cblocks[j].last_used = 0;
			clockX = clockX + 2;
			m.myCache.cblocks[i].data[wordIndex] = value;
			m.myCache.cblocks[i].last_used = 1;
			m.myCache.cblocks[i].valid = 1;
			clockX = clockX + 100;
			mm.blocks[addr].data[wordIndex] = value;
		}
		else
		{
			//printf("*******Miss in FULL Put");
			numMisses = numMisses + 1;
			clockX = clockX + 100;
			mm.blocks[addr].data[wordIndex] = value;
			for(int j=0; j<BLOCKS_IN_CACHE; j++) // reset cache last used values
				m.myCache.cblocks[j].last_used = 0;
			m.myCache.cblocks[i] = mm.blocks[addr];
			m.myCache.cblocks[i].tag = addr;
			m.myCache.cblocks[i].last_used = 1;
			m.myCache.cblocks[i].valid = 1;
		}
	}
}
