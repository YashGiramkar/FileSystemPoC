/**
 * @file          Sample_Format.c
 * @brief         Source file containing <Details>
 * @date          <Date of generating C file - DD/MM/YY>
 * @author        <Author of C file - Name [Initials]>
 * @copyright     Bajaj Auto Technology Limited (BATL)
 */

/******************************************************************************/
/*                                                                            */
/*                                  INCLUDES                                  */
/*                                                                            */
/******************************************************************************/
#include "FileSysManager.h"
#include "FileSysManagerFSM.h"
#include <ff.h>

/******************************************************************************/
/*                                                                            */
/*                                  DEFINES                                   */
/*                                                                            */
/******************************************************************************/
/**
 * @def           <Define name>
 * @brief         <Define details>.
 */

#define FSMGR_THREAD_STACK_SIZE              4096
#define FSMGR_THREAD_PRIORITY                5

// Logging module for FileSysManager
LOG_MODULE_REGISTER(FSMGR);

K_MSGQ_DEFINE(fs_msgq, sizeof(FileSysMessage_T), 16, 4);

// Structure to hold FATFS and file information
static FATFS st_FatFs;

/******************************************************************************/
/*                                                                            */
/*                                   ENUMS                                    */
/*                                                                            */
/******************************************************************************/
// Definition of all the enums
/**
 * @enum          <Enum name>
 * @brief         <Enum details>.
 */

// Declarations of all the enum variables
/**
 * @var           <Variable name>
 * @brief         <Variable details>.
 */

/******************************************************************************/
/*                                                                            */
/*                                 STRUCTURES                                 */
/*                                                                            */
/******************************************************************************/
// Definition of all the structures
/**
 * @struct        <Structure name>
 * @brief         <Structure details>.
 */

// Declarations of all the structure variables
/**
 * @var           <Variable name>
 * @brief         <Variable details>.
 */
// File system mount info structure
static struct fs_mount_t mount_info = {
    .type = FS_FATFS,
    .fs_data = &st_FatFs,
    .storage_dev = (void *)FAT_DRIVE_NAME,
    .mnt_point = FAT_MOUNT_POINT
};

/******************************************************************************/
/*                                                                            */
/*                                   UNIONS                                   */
/*                                                                            */
/******************************************************************************/
// Definition of all the unions
/**
 * @union         <Union name>
 * @brief         <Union details>.
 */

// Declarations of all the union variables
/**
 * @var           <Variable name>
 * @brief         <Variable details>.
 */

/******************************************************************************/
/*                                                                            */
/*                       PRIVATE FUNCTION DECLARATIONS                        */
/*                                                                            */
/******************************************************************************/
static int si_FileSystemMount(void);

/******************************************************************************/
/*                                                                            */
/*                              EXTERN VARIABLES                              */
/*                                                                            */
/******************************************************************************/

/******************************************************************************/
/*                                                                            */
/*                              PUBLIC VARIABLES                              */
/*                                                                            */
/******************************************************************************/
/**
 * @var           <Variable name>
 * @brief         <Variable details>.
 */

/******************************************************************************/
/*                                                                            */
/*                             PRIVATE VARIABLES                              */
/*                                                                            */
/******************************************************************************/
/**
 * @var           <Variable name>
 * @brief         <Variable details>.
 */
static FileSysManagerCTX_T sst_FSMGRContext;

// File information structure for the file we will create and read from
// struct fs_file_t st_FileInfo;

/******************************************************************************/
/*                                                                            */
/*                              EXTERN FUNCTIONS                              */
/*                                                                            */
/******************************************************************************/

/******************************************************************************/
/*                                                                            */
/*                        PRIVATE FUNCTION DEFINITIONS                        */
/*                                                                            */
/******************************************************************************/
/**
 * @private       <Function name>
 * @brief         <Function details>.
 * @param[in]     <Input parameter details>.
 * @param[out]    <Output parameter details>.
 * @param[inout]  <Input-Output parameter details>.
 * @return        <Return details>.
 */

static int si_FileSystemMount(void)
{
   // Variable to hold return codes from file system operations
   int i_RetCode;

   // Attempt to mount the file system
   i_RetCode = fs_mount(&mount_info);

      // If mounting fails, attempt to format the drive and mount again
   if (i_RetCode < 0)
   {
      LOG_INF("Mount failed (%d), formatting...", i_RetCode);

      // Format the drive with FATFS
      i_RetCode = fs_mkfs(FS_FATFS, (uintptr_t)(FAT_DRIVE_NAME ":"), NULL, 0);
      // If formatting fails, log the error and exit
      if (i_RetCode < 0)
      {
         LOG_ERR("Format failed (%d)", i_RetCode);
         return i_RetCode;
      }

      // Try mounting again after formatting
      i_RetCode = fs_mount(&mount_info);
      // If mounting still fails after formatting, log the error and exit
      if (i_RetCode < 0)
      {
         LOG_ERR("Mount failed after format (%d)", i_RetCode);
         return i_RetCode;
      }
   }
   return i_RetCode;
}


static void fs_thread(void *a, void *b, void *c)
{
   // Variable to hold return codes from file system operations
   int i_RetCode;

   // Mount the file system
   i_RetCode = si_FileSystemMount();
   if(i_RetCode < 0)
   {
      // Log that the FATFS has been successfully mounted
      LOG_ERR("FATFS Mounting Failed!");
   }
   else
   {
      // Log that the FATFS has been successfully mounted
      LOG_INF("FATFS Mounted!");
   }

   // Initialize the file system manager FSM
   gv_FileSysManagerFSMInit(&sst_FSMGRContext);

   while (1)
   {
      k_msgq_get(&fs_msgq, &sst_FSMGRContext.st_currentMsg, K_FOREVER);
      gv_FileSysManagerFSMRun(&sst_FSMGRContext);
   }
}
K_THREAD_DEFINE(fs_tid, FSMGR_THREAD_STACK_SIZE,
                fs_thread, NULL, NULL, NULL,
                FSMGR_THREAD_PRIORITY, 0, 0);




/******************************************************************************/
/*                                                                            */
/*                        PUBLIC FUNCTION DEFINITIONS                         */
/*                                                                            */
/******************************************************************************/
/**
 * @public        <Function name>
 * @brief         <Function details>.
 * @param[in]     <Input parameter details>.
 * @param[out]    <Output parameter details>.
 * @param[inout]  <Input-Output parameter details>.
 * @return        <Return details>.
 */
struct k_msgq *gstpt_FSMGR_GetMsgQ(void)
{
    return &fs_msgq;
}

/**
 * Copyright(c) Bajaj Auto Technology Limited (BATL) as an unpublished work.
 * THIS SOFTWARE AND/OR MATERIAL IS THE PROPERTY OF BATL.
 * ALL USE, DISCLOSURE, AND/OR REPRODUCTION NOT SPECIFICALLY AUTHORIZED BY
 * BATL IS PROHIBITED.
 *
 * @author:<Author of C file - Name [Initials]>
 */
