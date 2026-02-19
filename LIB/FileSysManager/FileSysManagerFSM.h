/**
 * @file          Sample_Format.h
 * @brief         Header file containing <Details>
 * @date          <Date of generating H file - DD/MM/YY>
 * @author        <Author of H file - Name [Initials]>
 * @copyright     Bajaj Auto Technology Limited (BATL)
 */

#ifndef FS_MGR_FSM_H
#define FS_MGR_FSM_H

/******************************************************************************/
/*                                                                            */
/*                                  INCLUDES                                  */
/*                                                                            */
/******************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include "TransferMsgTypes.h"

/******************************************************************************/
/*                                                                            */
/*                                  DEFINES                                   */
/*                                                                            */
/******************************************************************************/
/**
 * @def           <Define name>
 * @brief         <Define details>.
 */
// FATFS Drive Name
#define FAT_DRIVE_NAME                       "FLASH_DISK"
// Mount point for FATFS
#define FAT_MOUNT_POINT                      "/" FAT_DRIVE_NAME ":"
/******************************************************************************/
/*                                                                            */
/*                                   ENUMS                                    */
/*                                                                            */
/******************************************************************************/
/**
 * @enum          <Enum name>
 * @brief         <Enum details>.
 */

/******************************************************************************/
/*                                                                            */
/*                                 STRUCTURES                                 */
/*                                                                            */
/******************************************************************************/
/**
 * @struct        <Structure name>
 * @brief         <Structure details>.
 */
typedef struct{
   struct smf_ctx smf;                       /** Mandatory field for Zephyr SMF */

   /* Custom Data begins here*/
   struct fs_file_t file;                    /** File Pointer and information */
   FileSysMessage_T st_currentMsg;           /** Current Command and Data */
   uint32_t u32_byteWritten;                 /** Data bytes received so far*/
   uint32_t u32_totalExpectedBytes;          /** Total number of bytes expected*/
   bool b_fileOpenStatus;                    /** File Open Status */
   char as8_currentDir[FS_MAX_CHUNK_SIZE];   /** Active directory for relative file paths */
   char as8_activeFile[FS_MAX_CHUNK_SIZE];   /** Last opened file path */
}FileSysManagerCTX_T;

/******************************************************************************/
/*                                                                            */
/*                                   UNIONS                                   */
/*                                                                            */
/******************************************************************************/
/**
 * @union         <Union name>
 * @brief         <Union details>.
 */

/******************************************************************************/
/*                                                                            */
/*                              EXTERN VARIABLES                              */
/*                                                                            */
/******************************************************************************/

/******************************************************************************/
/*                                                                            */
/*                              EXTERN FUNCTIONS                              */
/*                                                                            */
/******************************************************************************/
void gv_FileSysManagerFSMInit(FileSysManagerCTX_T *ctx);
void gv_FileSysManagerFSMRun(FileSysManagerCTX_T *ctx);

#endif //!FS_MGR_FSM_H

/**
 * Copyright(c) Bajaj Auto Technology Limited (BATL) as an unpublished work.
 * THIS SOFTWARE AND/OR MATERIAL IS THE PROPERTY OF BATL.
 * ALL USE, DISCLOSURE, AND/OR REPRODUCTION NOT SPECIFICALLY AUTHORIZED BY
 * BATL IS PROHIBITED.
 *
 * @author:<Author of H file - Name [Initials]>
 */
