/**
 * @file          TransferMsgTypes.h
 * @brief         Header file containing details and data types of structures
 *                associated with file and data transfer
 * @date          18/02/26
 * @author        Yash Sunil Giramkar [YSG]
 * @copyright     Bajaj Auto Technology Limited (BATL)
 */

#ifndef TRANSFER_MSG_H
#define TRANSFER_MSG_H

/******************************************************************************/
/*                                                                            */
/*                                  INCLUDES                                  */
/*                                                                            */
/******************************************************************************/

#include <zephyr/kernel.h>

/******************************************************************************/
/*                                                                            */
/*                                  DEFINES                                   */
/*                                                                            */
/******************************************************************************/
/**
 * @def           FS_MAX_CHUNK_SIZE
 * @brief         Maximum chunk of data that can be received in one transfer
 *                between BT FSM and File Manager FSM
 */
#define FS_MAX_CHUNK_SIZE                    240

/**
 * @def MAX_FILENAME_LEN
 * @brief
 */
#define MAX_FILENAME_LEN                     64

/**
 * @def MAX_PATH_LEN
 * @brief
 */
#define MAX_PATH_LEN                         3

/******************************************************************************/
/*                                                                            */
/*                                   ENUMS                                    */
/*                                                                            */
/******************************************************************************/
/**
 * @enum          FileSysManagerEvents_E
 * @brief         Enum to hold status of each event of file system manager.
 */
typedef enum {
   FSC_OPEN_DIR,
   FSC_OPEN_FILE,
   FSC_WRITE_DATA,
   FSC_READ_FILE,
   FSC_CLOSE_FILE,
   FSC_ABORT
} FileSysCommand_E;

/******************************************************************************/
/*                                                                            */
/*                                 STRUCTURES                                 */
/*                                                                            */
/******************************************************************************/
/**
 * @struct        FileSysMessage_T
 * @brief         Structure to hold details of message queue object passed
 *                between BT FSM and File System Manager FSM.
 */
typedef struct {
   FileSysCommand_E e_command;

   /* Used for Data message as well as File and Directory creation*/
   uint32_t u32_sizeOfData;
   // Data field can either contain Directory name, File name or data to write
   uint8_t u8_data[FS_MAX_CHUNK_SIZE];
} FileSysMessage_T;


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

#endif //!TRANSFER_MSG_H

/**
 * Copyright(c) Bajaj Auto Technology Limited (BATL) as an unpublished work.
 * THIS SOFTWARE AND/OR MATERIAL IS THE PROPERTY OF BATL.
 * ALL USE, DISCLOSURE, AND/OR REPRODUCTION NOT SPECIFICALLY AUTHORIZED BY
 * BATL IS PROHIBITED.
 *
 * @author:Yash Sunil Giramkar [YSG]
 */












