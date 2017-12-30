/**
 * \addtogroup      ESP_MEM
 * \{
 *
 * Memory manager is light-weight implementation of malloc and free functions in standard C language.
 *
 * It uses <b>FIRST-FIT</b> allocation algorithm meaning it uses the first free region which suits available length of memory.
 *
 * On top of everything, it supports different memory regions which means user doesn't need to provide one full block of memory.
 *
 * \par             Memory regions
 *
 * Memory regions are a way how to provide splitted memory organizations.
 *
 * Consider use of external RAM memory (usually SDRAM), where user would like to use it as dynamic memory (heap) 
 * together with internal RAM on target device.
 *
 * By defining start address and length of regions, user can use unlimited number of regions.
 *
 * \note            When assigning regions, next region must always have greater memory address than one before.
 *
 * \note            The only limitation of regions is that region must be addressible in memory space.
 *                  If external memory is used, it must have memory mapping hardware to take care of addressing.
 *
 * Examples of defining 2 regions, one in internal memory, second on external SDRAM memory
 *
 * \code{c}
/*
 * This part should be done in ll initialization function only once on startup
 * Check ESP_LL part of library for more info
 */
 
/*
 * We can simply create a big array variable which will be linked to internal memory by linker 
 */
uint8_t mem_int[0x1000];

/*
 * Define memory regions for allocating algorithm,
 * make sure regions are in correct order for memory location
 */
esp_mem_region_t mem_regions[] = {
    { mem_int, sizeof(mem_int) },               /* Set first memory region to internal memory of length 0x1000 bytes */ 
    { (void *)0xC0000000, 0x8000 },             /* External heap memory is located on 0xC0000000 and has 0x8000 bytes of memory */ 
};

/*
 * On startup, user must call function to assign memory regions
 */
esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions));
\endcode
 *
 * \}
 */
/**
 * \addtogroup      ESP_MEM
 * \{
 * \note            Even with multiple regions, maximal allocation size is length of biggest region.
 *                  In case of example, we have <b>0x9000</b> bytes of memory but theoretically only <b>0x8000</b> may be allocated. 
 *                  Practically maximal value is little lower due to header values required to track blocks.
 *
 * \par             Allocating memory
 *
 * Now once we have set regions, we can proceed with allocating the memory.
 * On beginning, all regions are big blocks of free memory (number of big blocks equals number of regions as all regions are free)
 * and these blocks will be smaller, depends on allocating and freeing.
 *
 * Image below shows how memory structure looks like after some time of allocating and freeing memory regions.
 * <b>WHITE</b> blocks are free regions (ready to be allocated) and <b>RED</b> blocks are already allocated and used by user.
 *
 * When block is allocated, it is removed from list of free blocks so that we only track list of free blocks.
 * This is visible with connections of white blocks together.
 *
 * \image html memory_manager_structure.svg Memory structure after allocating and freeing.
 *
 * \par             Freeing memory
 * 
 * When we try to free used memory, it is set as free and then inserted between 2 free blocks
 * which is visible on first part of image below.
 *
 * Later, we also decide to free block which is just between 2 free blocks.
 * When we do it, manager will detect that there is free memory before and after used one
 * and will mark everything together into one big block, which is visible on second part of image below.
 *
 * \image html memory_manager_structure_freeing.svg Memory structure after freeing 2 blocks
 *
 * \}
 */