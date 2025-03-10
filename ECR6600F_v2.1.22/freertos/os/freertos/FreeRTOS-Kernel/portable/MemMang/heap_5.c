/*
 * FreeRTOS Kernel V10.4.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 * 1 tab == 4 spaces!
 */

/*
 * A sample implementation of pvPortMalloc() that allows the heap to be defined
 * across multiple non-contigous blocks and combines (coalescences) adjacent
 * memory blocks as they are freed.
 *
 * See heap_1.c, heap_2.c, heap_3.c and heap_4.c for alternative
 * implementations, and the memory management pages of https://www.FreeRTOS.org
 * for more information.
 *
 * Usage notes:
 *
 * vPortDefineHeapRegions() ***must*** be called before pvPortMalloc().
 * pvPortMalloc() will be called if any task objects (tasks, queues, event
 * groups, etc.) are created, therefore vPortDefineHeapRegions() ***must*** be
 * called before any other objects are defined.
 *
 * vPortDefineHeapRegions() takes a single parameter.  The parameter is an array
 * of HeapRegion_t structures.  HeapRegion_t is defined in portable.h as
 *
 * typedef struct HeapRegion
 * {
 *	uint8_t *pucStartAddress; << Start address of a block of memory that will be part of the heap.
 *	size_t xSizeInBytes;	  << Size of the block of memory.
 * } HeapRegion_t;
 *
 * The array is terminated using a NULL zero sized region definition, and the
 * memory regions defined in the array ***must*** appear in address order from
 * low address to high address.  So the following is a valid example of how
 * to use the function.
 *
 * HeapRegion_t xHeapRegions[] =
 * {
 *  { ( uint8_t * ) 0x80000000UL, 0x10000 }, << Defines a block of 0x10000 bytes starting at address 0x80000000
 *  { ( uint8_t * ) 0x90000000UL, 0xa0000 }, << Defines a block of 0xa0000 bytes starting at address of 0x90000000
 *  { NULL, 0 }                << Terminates the array.
 * };
 *
 * vPortDefineHeapRegions( xHeapRegions ); << Pass the array into vPortDefineHeapRegions().
 *
 * Note 0x80000000 is the lower address so appears in the array first.
 *
 */
#include <stdlib.h>
#include <string.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
 * all the API functions to use the MPU wrappers.  That should only be done when
 * task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"
#include "oshal.h"
#include "debug_core.h"
#include "pit.h"
#include "chip_clk_ctrl.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
    #error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE    ( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE         ( ( size_t ) 8 )

#ifdef CONFIG_HEAP_BOUNDARY_CHECK
#define heapBOUNDARYSIZE sizeof(uint32_t)
#else
#define heapBOUNDARYSIZE 0
#endif
#define pdHEAPBOUNDARY_CHECK_VALUE    0x5a5a5a5aUL
#define pdHEAPBOUNDARY_POSITION(start,size)	  ((uint32_t)start+(uint32_t)size-heapBOUNDARYSIZE)


/* Define the linked list structure.  This is used to link free blocks in order
 * of their memory address. */
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK * pxNextFreeBlock; /*<< The next free block in the list. */
    size_t xBlockSize;                     /*<< The size of the free block. */
#if defined(CONFIG_HEAP_DEBUG)
	uint16_t usLine;
	uint16_t usTaskID;
	char *pcFileName;
#endif
} BlockLink_t;


#if defined(CONFIG_HEAP_DEBUG)

BlockLink_t xHeapUsed={0};


#endif

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t * pxBlockToInsert );

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
 * block must by correctly byte aligned. */
static const size_t xHeapStructSize = ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, * pxEnd = NULL;

/* Keeps track of the number of calls to allocate and free memory as well as the
 * number of free bytes remaining, but says nothing about fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;
static size_t xNumberOfSuccessfulAllocations = 0;
static size_t xNumberOfSuccessfulFrees = 0;
size_t xTotalValidHeapSize = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
 * member of an BlockLink_t structure is set then the block belongs to the
 * application.  When the bit is free the block is still part of the free heap
 * space. */
static size_t xBlockAllocatedBit = 0;

/*-----------------------------------------------------------*/

void *pvPortCalloc( size_t nmemb, size_t size )
{
    void *pvReturn;
	extern void *memset(void *s, int ch, size_t n);
    pvReturn = pvPortMalloc( nmemb*size );
    if (pvReturn)
    {
        memset(pvReturn, 0, nmemb*size);
    }
    return pvReturn;
}
unsigned int heap_lock_max_time;
#define LOCK_LIMIT_TIME_MS CHIP_CLOCK_APB/1000
#if defined(CONFIG_HEAP_DEBUG)
void * pvPortMallocDebug( size_t xWantedSize ,char *pcFileName, uint32_t ulLine)
#else
void * pvPortMalloc( size_t xWantedSize )
#endif
{
    BlockLink_t * pxBlock, * pxPreviousBlock, * pxNewBlockLink;
    void * pvReturn = NULL;
    unsigned int flag=0;
    unsigned int time_b=0;
    unsigned int time_a=0;

    /* The heap must be initialised before the first call to
     * prvPortMalloc(). */
    configASSERT( pxEnd );

    //vTaskSuspendAll();
    flag=system_irq_save();
    time_b=drv_pit_get_tick();
    {
        /* Check the requested block size is not so large that the top bit is
         * set.  The top bit of the block size member of the BlockLink_t structure
         * is used to determine who owns the block - the application or the
         * kernel, so it must be free. */
        if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
        {
            /* The wanted size is increased so it can contain a BlockLink_t
             * structure in addition to the requested amount of bytes. */
            if( xWantedSize > 0 )
            {
                xWantedSize += xHeapStructSize;
				xWantedSize += heapBOUNDARYSIZE;

                /* Ensure that blocks are always aligned to the required number
                 * of bytes. */
                if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
                {
                    /* Byte alignment required. */
                    xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
            {
                /* Traverse the list from the start	(lowest address) block until
                 * one	of adequate size is found. */
                pxPreviousBlock = &xStart;
                pxBlock = xStart.pxNextFreeBlock;

                while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
                {
                    pxPreviousBlock = pxBlock;
                    pxBlock = pxBlock->pxNextFreeBlock;
                }

                /* If the end marker was reached then a block of adequate size
                 * was	not found. */
                if( pxBlock != pxEnd )
                {
                    /* Return the memory space pointed to - jumping over the
                     * BlockLink_t structure at its start. */
                    pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

                    /* This block is being returned for use so must be taken out
                     * of the list of free blocks. */
                    pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                    /* If the block is larger than required it can be split into
                     * two. */
                    if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
                    {
                        /* This block is to be split into two.  Create a new
                         * block following the number of bytes requested. The void
                         * cast is used to prevent byte alignment warnings from the
                         * compiler. */
                        pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );

                        /* Calculate the sizes of two blocks split from the
                         * single block. */
                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                        pxBlock->xBlockSize = xWantedSize;

                        /* Insert the new block into the list of free blocks. */
                        prvInsertBlockIntoFreeList( ( pxNewBlockLink ) );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    xFreeBytesRemaining -= pxBlock->xBlockSize;

                    if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
                    {
                        xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
					if(heapBOUNDARYSIZE)
                    {
						uint32_t *pulBoundaryPos=(uint32_t *)pdHEAPBOUNDARY_POSITION(pxBlock,pxBlock->xBlockSize);
						*pulBoundaryPos=pdHEAPBOUNDARY_CHECK_VALUE;
                    }
                    /* The block is being returned - it is allocated and owned
                     * by the application and has no "next" block. */
					#if defined(CONFIG_HEAP_DEBUG)
						{
							pxBlock->pxNextFreeBlock=xHeapUsed.pxNextFreeBlock;
							xHeapUsed.pxNextFreeBlock=pxBlock;
							xHeapUsed.xBlockSize+=pxBlock->xBlockSize;
							xHeapUsed.pcFileName+=1;
							pxBlock->pcFileName = pcFileName;
							pxBlock->usTaskID = ucGetTaskID(NULL);
							pxBlock->usLine =(uint16_t)ulLine;
							vAddTaskHeap(pxBlock->usTaskID,pxBlock->xBlockSize,1);
						}
					#else
						pxBlock->pxNextFreeBlock = NULL;
					#endif
                    pxBlock->xBlockSize |= xBlockAllocatedBit;
                    xNumberOfSuccessfulAllocations++;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceMALLOC( pvReturn, xWantedSize );
    }
    //( void ) xTaskResumeAll();
    time_a=drv_pit_get_tick();
    if(heap_lock_max_time<(time_a-time_b))
    {
    	heap_lock_max_time=time_a-time_b;
    }
	system_irq_restore(flag);
	if(time_a-time_b>LOCK_LIMIT_TIME_MS)
	{
		os_printf(LM_OS,LL_INFO,"malloc:b %d, a %d\r\n",time_b,time_a);
	}
    #if ( configUSE_MALLOC_FAILED_HOOK == 1 )
        {
            if( pvReturn == NULL )
            {
                extern void vApplicationMallocFailedHook( void );
                vApplicationMallocFailedHook();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
    #endif /* if ( configUSE_MALLOC_FAILED_HOOK == 1 ) */

    return pvReturn;
}

/*-----------------------------------------------------------*/
void *pvPortReMalloc(void *pvOld, size_t xNewSize)
{
    uint8_t *puc = (uint8_t *)pvOld;
    void *pvNew = NULL;
    BlockLink_t *pxLink;
    size_t xOldSize;

    if (!pvOld) {
        return pvPortMalloc(xNewSize);
    }

    if (0 == xNewSize) {
        vPortFree(pvOld);
        return NULL;
    }

    if (!(pvNew = pvPortMalloc(xNewSize))) {
        return NULL; // should free pvOld here?
    }

    puc -= xHeapStructSize;
    pxLink = (void *)puc;
    configASSERT((pxLink->xBlockSize & xBlockAllocatedBit) != 0);

	#if !defined(CONFIG_HEAP_DEBUG)
	    configASSERT(pxLink->pxNextFreeBlock == NULL);
	#endif

	xOldSize = (pxLink->xBlockSize & ~xBlockAllocatedBit);
    memcpy(pvNew, pvOld, xOldSize <=  xNewSize ? xOldSize : xNewSize);

    vPortFree(pvOld);

    return pvNew;
}
/*-----------------------------------------------------------*/

void vPortFree( void * pv )
{
    uint8_t * puc = ( uint8_t * ) pv;
    BlockLink_t * pxLink;
    unsigned int flag;
    unsigned int time_b=0;
    unsigned int time_a=0;

    if( pv != NULL )
    {
        /* The memory being freed will have an BlockLink_t structure immediately
         * before it. */
        puc -= xHeapStructSize;

        /* This casting is to keep the compiler from issuing warnings. */
        pxLink = ( void * ) puc;

        /* Check the block is actually allocated. */
		#if defined(CONFIG_HEAP_DEBUG)
			{
				BlockLink_t *pxPreviousBlock,*pxBlock;

				//vTaskSuspendAll();
				flag=system_irq_save();
				time_b=drv_pit_get_tick();

				pxPreviousBlock=&xHeapUsed;
				pxBlock=pxPreviousBlock->pxNextFreeBlock;
				while(pxBlock)
				{
					if(pxBlock==pxLink)
					{
						pxPreviousBlock->pxNextFreeBlock=pxBlock->pxNextFreeBlock;
						pxBlock->pxNextFreeBlock=NULL;
						vAddTaskHeap(pxBlock->usTaskID,pxBlock->xBlockSize&~xBlockAllocatedBit,0);
						xHeapUsed.xBlockSize-=pxBlock->xBlockSize&~xBlockAllocatedBit;
						xHeapUsed.pcFileName-=1;
						break;
					}
					pxPreviousBlock=pxBlock;
					pxBlock=pxBlock->pxNextFreeBlock;
				}
		 		//( void ) xTaskResumeAll();
		 		time_a=drv_pit_get_tick();
				if(heap_lock_max_time<(time_a-time_b))
			    {
			    	heap_lock_max_time=time_a-time_b;
			    }
		 		system_irq_restore(flag);
				configASSERT( pxBlock != NULL );
				if(time_a-time_b>LOCK_LIMIT_TIME_MS)
				{
					os_printf(LM_OS,LL_INFO,"free stage1:b %d, a %d\r\n",time_b,time_a);
				}
			}
		#endif
        configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
        configASSERT( pxLink->pxNextFreeBlock == NULL );

        if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
        {
            if( pxLink->pxNextFreeBlock == NULL )
            {
                /* The block is being returned to the heap - it is no longer
                 * allocated. */
                pxLink->xBlockSize &= ~xBlockAllocatedBit;
                if(heapBOUNDARYSIZE)
                {
                	uint32_t *pulBoundaryPos=(uint32_t *)pdHEAPBOUNDARY_POSITION(pxLink, pxLink->xBlockSize);
					configASSERT( *pulBoundaryPos == pdHEAPBOUNDARY_CHECK_VALUE );
                }

                //vTaskSuspendAll();
                flag=system_irq_save();

                time_b=drv_pit_get_tick();
                {
                    /* Add this block to the list of free blocks. */
                    xFreeBytesRemaining += pxLink->xBlockSize;
                    traceFREE( pv, pxLink->xBlockSize );
                    prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
                    xNumberOfSuccessfulFrees++;
                }
                //( void ) xTaskResumeAll();
                time_a=drv_pit_get_tick();
				if(heap_lock_max_time<(time_a-time_b))
			    {
			    	heap_lock_max_time=time_a-time_b;
			    }
                system_irq_restore(flag);
                if(time_a-time_b>LOCK_LIMIT_TIME_MS)
				{
					os_printf(LM_OS,LL_INFO,"free stage 2:b %d, a %d\r\n",time_b,time_a);
				}
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
}
/*-----------------------------------------------------------*/

size_t xPortMemIsFree(void *mem)
{
	uint8_t * puc = ( uint8_t * ) mem;
    BlockLink_t * pxLink;


    puc -= xHeapStructSize;

    pxLink = ( void * ) puc;

    return (pxLink->xBlockSize & xBlockAllocatedBit)==0 ;
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
    return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize( void )
{
    return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t * pxBlockToInsert )
{
    BlockLink_t * pxIterator;
    uint8_t * puc;

    /* Iterate through the list until a block is found that has a higher address
     * than the block being inserted. */
    for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
    {
        /* Nothing to do here, just iterate to the right position. */
    }

    /* Do the block being inserted, and the block it is being inserted after
     * make a contiguous block of memory? */
    puc = ( uint8_t * ) pxIterator;

    if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
    {
        pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
        pxBlockToInsert = pxIterator;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    /* Do the block being inserted, and the block it is being inserted before
     * make a contiguous block of memory? */
    puc = ( uint8_t * ) pxBlockToInsert;

    if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
    {
        if( pxIterator->pxNextFreeBlock != pxEnd )
        {
            /* Form one big block from the two blocks. */
            pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
            pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
        }
        else
        {
            pxBlockToInsert->pxNextFreeBlock = pxEnd;
        }
    }
    else
    {
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
    }

    /* If the block being inserted plugged a gab, so was merged with the block
     * before and the block after, then it's pxNextFreeBlock pointer will have
     * already been set, and should not be set here as that would make it point
     * to itself. */
    if( pxIterator != pxBlockToInsert )
    {
        pxIterator->pxNextFreeBlock = pxBlockToInsert;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
}
/*-----------------------------------------------------------*/

void vPortDefineHeapRegions( const HeapRegion_t * const pxHeapRegions )
{
    BlockLink_t * pxFirstFreeBlockInRegion = NULL, * pxPreviousFreeBlock;
    size_t xAlignedHeap;
    size_t xTotalRegionSize, xTotalHeapSize = 0;
    BaseType_t xDefinedRegions = 0;
    size_t xAddress;
    const HeapRegion_t * pxHeapRegion;

    /* Can only call once! */
    configASSERT( pxEnd == NULL );

    pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );

    while( pxHeapRegion->xSizeInBytes > 0 )
    {
        xTotalRegionSize = pxHeapRegion->xSizeInBytes;

        /* Ensure the heap region starts on a correctly aligned boundary. */
        xAddress = ( size_t ) pxHeapRegion->pucStartAddress;

        if( ( xAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
        {
            xAddress += ( portBYTE_ALIGNMENT - 1 );
            xAddress &= ~portBYTE_ALIGNMENT_MASK;

            /* Adjust the size for the bytes lost to alignment. */
            xTotalRegionSize -= xAddress - ( size_t ) pxHeapRegion->pucStartAddress;
        }

        xAlignedHeap = xAddress;

        /* Set xStart if it has not already been set. */
        if( xDefinedRegions == 0 )
        {
            /* xStart is used to hold a pointer to the first item in the list of
             *  free blocks.  The void cast is used to prevent compiler warnings. */
            xStart.pxNextFreeBlock = ( BlockLink_t * ) xAlignedHeap;
            xStart.xBlockSize = ( size_t ) 0;
        }
        else
        {
            /* Should only get here if one region has already been added to the
             * heap. */
            configASSERT( pxEnd != NULL );

            /* Check blocks are passed in with increasing start addresses. */
            configASSERT( xAddress > ( size_t ) pxEnd );
        }

        /* Remember the location of the end marker in the previous region, if
         * any. */
        pxPreviousFreeBlock = pxEnd;

        /* pxEnd is used to mark the end of the list of free blocks and is
         * inserted at the end of the region space. */
        xAddress = xAlignedHeap + xTotalRegionSize;
        xAddress -= xHeapStructSize;
        xAddress &= ~portBYTE_ALIGNMENT_MASK;
        pxEnd = ( BlockLink_t * ) xAddress;
        pxEnd->xBlockSize = 0;
        pxEnd->pxNextFreeBlock = NULL;

        /* To start with there is a single free block in this region that is
         * sized to take up the entire heap region minus the space taken by the
         * free block structure. */
        pxFirstFreeBlockInRegion = ( BlockLink_t * ) xAlignedHeap;
        pxFirstFreeBlockInRegion->xBlockSize = xAddress - ( size_t ) pxFirstFreeBlockInRegion;
        pxFirstFreeBlockInRegion->pxNextFreeBlock = pxEnd;

        /* If this is not the first region that makes up the entire heap space
         * then link the previous region to this region. */
        if( pxPreviousFreeBlock != NULL )
        {
            pxPreviousFreeBlock->pxNextFreeBlock = pxFirstFreeBlockInRegion;
        }

        xTotalHeapSize += pxFirstFreeBlockInRegion->xBlockSize;

        /* Move onto the next HeapRegion_t structure. */
        xDefinedRegions++;
        pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );
    }

    xMinimumEverFreeBytesRemaining = xTotalHeapSize;
    xFreeBytesRemaining = xTotalHeapSize;
	xTotalValidHeapSize = xTotalHeapSize;

    /* Check something was actually defined before it is accessed. */
    configASSERT( xTotalHeapSize );

    /* Work out the position of the top bit in a size_t variable. */
    xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );
}
/*-----------------------------------------------------------*/

void vPortGetHeapStats( HeapStats_t * pxHeapStats )
{
    BlockLink_t * pxBlock;
    size_t xBlocks = 0, xMaxSize = 0, xMinSize = portMAX_DELAY; /* portMAX_DELAY used as a portable way of getting the maximum value. */

    vTaskSuspendAll();
    {
        pxBlock = xStart.pxNextFreeBlock;

        /* pxBlock will be NULL if the heap has not been initialised.  The heap
         * is initialised automatically when the first allocation is made. */
        if( pxBlock != NULL )
        {
            do
            {
                /* Increment the number of blocks and record the largest block seen
                 * so far. */
                xBlocks++;

                if( pxBlock->xBlockSize > xMaxSize )
                {
                    xMaxSize = pxBlock->xBlockSize;
                }

                /* Heap five will have a zero sized block at the end of each
                 * each region - the block is only used to link to the next
                 * heap region so it not a real block. */
                if( pxBlock->xBlockSize != 0 )
                {
                    if( pxBlock->xBlockSize < xMinSize )
                    {
                        xMinSize = pxBlock->xBlockSize;
                    }
                }

                /* Move to the next block in the chain until the last block is
                 * reached. */
                pxBlock = pxBlock->pxNextFreeBlock;
            } while( pxBlock != pxEnd );
        }
    }
    ( void ) xTaskResumeAll();

    pxHeapStats->xSizeOfLargestFreeBlockInBytes = xMaxSize;
    pxHeapStats->xSizeOfSmallestFreeBlockInBytes = xMinSize;
    pxHeapStats->xNumberOfFreeBlocks = xBlocks;

    taskENTER_CRITICAL();
    {
        pxHeapStats->xAvailableHeapSpaceInBytes = xFreeBytesRemaining;
        pxHeapStats->xNumberOfSuccessfulAllocations = xNumberOfSuccessfulAllocations;
        pxHeapStats->xNumberOfSuccessfulFrees = xNumberOfSuccessfulFrees;
        pxHeapStats->xMinimumEverFreeBytesRemaining = xMinimumEverFreeBytesRemaining;
    }
    taskEXIT_CRITICAL();
}

/*-----------------------------------------------------------*/

#if defined(CONFIG_HEAP_DEBUG)
#include "oshal.h"
void vTaskHeapStats(char* pcTaskName)
{

	uint8_t ucTaskID;

	BlockLink_t *pxBlock;
	uint32_t ulMaxFreeSize=0;
	uint32_t ulTotalFreeSize=0;
	uint32_t ulTotalAllocSize=0;
	int count=0;

	vTaskSuspendAll();

	pxBlock = xStart.pxNextFreeBlock;
	while(pxBlock != NULL)
	{
		if(ulMaxFreeSize<pxBlock->xBlockSize)
		{
			ulMaxFreeSize=pxBlock->xBlockSize;
		}
		ulTotalFreeSize+=pxBlock->xBlockSize;
		pxBlock = pxBlock->pxNextFreeBlock;

	}

	if(pcTaskName)
	{
		ucTaskID = ucGetTaskIDWithName(pcTaskName);
		if(ucTaskID==0xff)
		{
			pcTaskName=NULL;
		}
	}

	pxBlock=xHeapUsed.pxNextFreeBlock;
	while(pxBlock)
	{
		if(!pcTaskName||pxBlock->usTaskID==ucTaskID)
		{
			os_printf(LM_CMD,LL_INFO,"%3d:\t%-16s\t%s:%d,\tsize=%d\n",
                count,
                pcGetTaskName((uint8_t)pxBlock->usTaskID),
                pxBlock->pcFileName,
                pxBlock->usLine,
                (pxBlock->xBlockSize&~xBlockAllocatedBit));
			count++;
		}

		ulTotalAllocSize+=(pxBlock->xBlockSize &~xBlockAllocatedBit);
		pxBlock=pxBlock->pxNextFreeBlock;
	}

	if(NULL==pcTaskName)
	{
		os_printf(LM_CMD,LL_INFO,"history left %d, current left %d, max avaible %d\n",
            xMinimumEverFreeBytesRemaining, xFreeBytesRemaining, ulMaxFreeSize);
		os_printf(LM_CMD,LL_INFO,"total size %d, total free size %d, total malloc %d:(=%d=%d), total malloc count %d\n",
            xTotalValidHeapSize,
            ulTotalFreeSize,
            (xTotalValidHeapSize-ulTotalFreeSize),
            ulTotalAllocSize,
            xHeapUsed.xBlockSize,
            xHeapUsed.pcFileName);
	}

	( void ) xTaskResumeAll();

}

#endif
/*-----------------------------------------------------------*/
extern HeapRegion_t sysHeap[];


int mem_is_vaild(BlockLink_t * pxLink, HeapRegion_t *pxHeapRegion)
{
	size_t xDefinedRegions=0;
	size_t xBlockSize=pxLink->xBlockSize&~xBlockAllocatedBit;
	if(xBlockSize>(size_t)pxHeapRegion->xSizeInBytes)
	{
		return 0;
	}
	else if((size_t)pxLink+xBlockSize > (size_t)pxHeapRegion->pucStartAddress+(size_t)pxHeapRegion->xSizeInBytes)
	{
		return 0;
	}
	else if(pxLink->xBlockSize>xBlockAllocatedBit)
	{
		#if !defined(CONFIG_HEAP_DEBUG)
			if(pxLink->pxNextFreeBlock)
			{
				return 0;
			}
		#endif

		if(heapBOUNDARYSIZE)
		{
			uint32_t *pulBoundaryPos=(uint32_t *)pdHEAPBOUNDARY_POSITION(pxLink, xBlockSize);
			if(*pulBoundaryPos != pdHEAPBOUNDARY_CHECK_VALUE)
			{
				return 0;
			}
		}

	}
	pxLink=pxLink->pxNextFreeBlock;
	xBlockSize=pxLink->xBlockSize&~xBlockAllocatedBit;

	if(pxLink)
	{
		pxHeapRegion = &sysHeap[xDefinedRegions];
		while( pxHeapRegion->xSizeInBytes > 0 )
		{
			if((size_t)pxLink>= (size_t)pxHeapRegion->pucStartAddress && (size_t)pxLink<= (size_t)pxHeapRegion->pucStartAddress+(size_t)pxHeapRegion->xSizeInBytes)
			{
				if((size_t)pxLink+xBlockSize<=(size_t)pxHeapRegion->pucStartAddress+(size_t)pxHeapRegion->xSizeInBytes)
				{
					return 1;
				}
				else
				{
					return 0;
				}
			}
			xDefinedRegions++;
			pxHeapRegion = &( sysHeap[ xDefinedRegions ] );
		}
		return 0;
	}

	return 1;

}



/*-----------------------------------------------------------*/

#if defined(CONFIG_HEAP_CHECK_WHEN_TASK_SWITCHING)
uint32_t ulHeapCheckEnable=1;
#else
uint32_t ulHeapCheckEnable=0;   // Default 0: full memory detection is not performed, which is time-consuming
#endif
volatile BlockLink_t * pxErrorBlock;

void vHeapCheckInTaskSwitch()
{
	if(0==ulHeapCheckEnable)  // Default 0: breake here
	{
		return ;
	}

	BaseType_t xDefinedRegions = 0;
	HeapRegion_t *pxHeapRegion;
	BlockLink_t * pxLink;
	size_t xAddress;
	size_t xTotalRegionSize;

	pxHeapRegion = &( sysHeap[ xDefinedRegions ] );
	xAddress = ( size_t ) pxHeapRegion->pucStartAddress;
	xAddress += ( portBYTE_ALIGNMENT - 1 );
	xAddress &= ~portBYTE_ALIGNMENT_MASK;
	/* Adjust the size for the bytes lost to alignment. */
	xTotalRegionSize -= xAddress - ( size_t ) pxHeapRegion->pucStartAddress;

	pxLink=(BlockLink_t *)xAddress;


	while( pxHeapRegion->xSizeInBytes > 0 )
	{
		while(pxLink->xBlockSize)
		{

			if(0==mem_is_vaild(pxLink,pxHeapRegion))
			{
				os_printf(LM_OS, LL_ASSERT, "***ERROR*** heap 0x%x error has been detected,please ramdump...\r\n",pxLink);

				portDISABLE_INTERRUPTS();
				pxErrorBlock=pxLink;

				vApplicationStackOverflowCheck((int)xTaskGetCurrentTaskHandle());

				trap_Heap_Error((int)xTaskGetCurrentTaskHandle(),1);
			}

			pxLink=(BlockLink_t *)((size_t)pxLink+(size_t)(pxLink->xBlockSize&~xBlockAllocatedBit));

		}
		xDefinedRegions++;
		pxHeapRegion = &( sysHeap[ xDefinedRegions ] );
		xAddress = ( size_t ) pxHeapRegion->pucStartAddress;
		xAddress += ( portBYTE_ALIGNMENT - 1 );
		xAddress &= ~portBYTE_ALIGNMENT_MASK;
		/* Adjust the size for the bytes lost to alignment. */
		xTotalRegionSize -= xAddress - ( size_t ) pxHeapRegion->pucStartAddress;
		pxLink=(BlockLink_t *)xAddress;

	}
}



