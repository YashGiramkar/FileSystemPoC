/**
 * @file          FileSysManagerFSM.c
 * @brief         File system manager state machine implementation
 */

/******************************************************************************/
/*                                  INCLUDES                                  */
/******************************************************************************/
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>

#include "FileSysManagerFSM.h"

/******************************************************************************/
/*                                  DEFINES                                   */
/******************************************************************************/
/**
 * @def           Logger Module for File System Manager FSM
 * @brief         This define is used to register the logging module for the File System Manager FSM.
 */
LOG_MODULE_REGISTER(FSMGR_FSM);

/**
 * @def           FSMGR_MAX_PATH_BUF
 * @brief         Defines the maximum buffer size for file paths in the File
 *                System Manager FSM. This is set to the maximum chunk size
 *                defined in TransferMsgTypes to ensure that file paths can
 *                be safely constructed from incoming messages without exceeding
 *                buffer limits.
 */
#define FSMGR_MAX_PATH_BUF                   FS_MAX_CHUNK_SIZE

/******************************************************************************/
/*                                   ENUMS                                    */
/******************************************************************************/
/**
 * @enum          e_FSMGR_FSM_states
 * @brief         Enum to maintain the various states of the File System Manager
 *                FSM.
 */
enum e_FSMGR_FSM_states
{
   STATE_IDLE,
   STATE_CREATE_DIR,
   STATE_CREATE_FILE,
   STATE_WRITE_FILE,
   STATE_READ_FILE,
   STATE_CLOSE,
   STATE_FAILED,
};

/******************************************************************************/
/*                       PRIVATE FUNCTION DECLARATIONS                        */
/******************************************************************************/

/* State functions associated with state machine starts*/
static void sv_IdleEntry(void *vptr);
static enum smf_state_result se_IdleRun(void *vptr);

static void sv_CreateDIREntry(void *vptr);
static enum smf_state_result se_CreateDIRRun(void *vptr);

static void sv_CreateFileEntry(void *vptr);
static enum smf_state_result se_CreateFileRun(void *vptr);

static void sv_WriteFileEntry(void *vptr);
static enum smf_state_result se_WriteFileRun(void *vptr);

static void sv_ReadFileEntry(void *vptr);
static enum smf_state_result se_ReadFileRun(void *vptr);

static void sv_CloseFileEntry(void *vptr);
static enum smf_state_result se_CloseFileRun(void *vptr);

static void sv_OpFailedEntry(void *vptr);
static enum smf_state_result se_OpFailedRun(void *vptr);

/* State functions associated with state machine ends*/

static void sv_CloseIfOpen(FileSysManagerCTX_T *stpt_ctx);
static int si_BuildPathFromMsg(FileSysManagerCTX_T *stpt_ctx,
                               bool b_createDirectory,
                               char *cpt_builtPath,
                               size_t s_builtPathMaxSize);
static void sv_LogReadData(const uint8_t *u8pt_data, uint32_t u32_len);

/******************************************************************************/
/*                             PRIVATE VARIABLES                              */
/******************************************************************************/
/**
 * @struct        state_table
 * @brief         The state table for the File System Manager FSM.
 */
static const struct smf_state state_table[] = {
    [STATE_IDLE]        = SMF_CREATE_STATE(sv_IdleEntry, se_IdleRun, NULL, NULL, NULL),
    [STATE_CREATE_DIR]  = SMF_CREATE_STATE(sv_CreateDIREntry, se_CreateDIRRun, NULL, NULL, NULL),
    [STATE_CREATE_FILE] = SMF_CREATE_STATE(sv_CreateFileEntry, se_CreateFileRun, NULL, NULL, NULL),
    [STATE_WRITE_FILE]  = SMF_CREATE_STATE(sv_WriteFileEntry, se_WriteFileRun, NULL, NULL, NULL),
    [STATE_READ_FILE]   = SMF_CREATE_STATE(sv_ReadFileEntry, se_ReadFileRun, NULL, NULL, NULL),
    [STATE_CLOSE]       = SMF_CREATE_STATE(sv_CloseFileEntry, se_CloseFileRun, NULL, NULL, NULL),
    [STATE_FAILED]      = SMF_CREATE_STATE(sv_OpFailedEntry, se_OpFailedRun, NULL, NULL, NULL),
};

/******************************************************************************/
/*                        PRIVATE FUNCTION DEFINITIONS                        */
/******************************************************************************/
/**
 * @private       sv_CloseIfOpen
 * @brief         Closes the file if it is open.
 * @param[in]     stpt_ctx - The context of the File System Manager.
 * @param[out]    None.
 * @param[inout]  None.
 * @return        None.
 */
static void sv_CloseIfOpen(FileSysManagerCTX_T *stpt_ctx)
{
   //  Check if the file is currently open
   if (stpt_ctx->b_fileOpenStatus)
   {
      // Close the file and update the open status
      (void)fs_close(&stpt_ctx->file);
      stpt_ctx->b_fileOpenStatus = false;
   }
}

/**
 * @private       si_BuildPathFromMsg
 * @brief         Builds a file or directory path based on the incoming message
 *                and the current context.
 * @param[in]     stpt_ctx - The context of the File System Manager.
 * @param[in]     b_createDirectory - Whether we are creating a directory.
 * @param[out]    None
 * @param[inout]  cpt_builtPath - Pointer to output built path
 * @return        0 on success, negative error code on failure.
 */
static int si_BuildPathFromMsg(FileSysManagerCTX_T *stpt_ctx,
                               bool b_createDirectory,
                               char *cpt_builtPath,
                               size_t s_builtPathMaxSize)
{
   char c_tempCharString[FSMGR_MAX_PATH_BUF];
   const char *ccpt_nameStart;
   const char *ccpt_basePath;
   int i_retVal;
   uint32_t u32_length = stpt_ctx->st_currentMsg.u32_sizeOfData;
   bool b_isAbsoluteFsPath;

   // Ensure that the incoming data size does not exceed our temporary buffer
   if (u32_length >= sizeof(c_tempCharString))
   {
      // If it does, then truncate the length to fit in our buffer
      u32_length = sizeof(c_tempCharString) - 1U;
   }
   else
   {
      // Do nothing, the length is valid
   }

   // Copy the received data into temporary buffer
   if (u32_length > 0U)
   {
      memcpy(c_tempCharString, stpt_ctx->st_currentMsg.u8_data, u32_length);
   }
   else
   {
      return -EINVAL;
   }

   // Null-terminate the temporary string to ensure it is a valid C-string
   c_tempCharString[u32_length] = '\0';

   // Check if the path is an absolute path (starts with '/' and contains ':')
   b_isAbsoluteFsPath = ((c_tempCharString[0] == '/') && (strstr(c_tempCharString, ":") != NULL));
   if (b_isAbsoluteFsPath)
   {
      // If it is an absolute path, we can directly copy it to the output buffer
      i_retVal = snprintf(cpt_builtPath, s_builtPathMaxSize, "%s", c_tempCharString);
      return (i_retVal > 0 && (size_t)i_retVal < s_builtPathMaxSize) ? 0 : -ENAMETOOLONG;
   }

   // If it is not an absolute path, we need to determine the base path to prepend
   if (c_tempCharString[0] == '/')
   {
      // If the path starts with '/', we treat it as relative to the FAT mount point
      ccpt_nameStart = &c_tempCharString[1];
      ccpt_basePath = FAT_MOUNT_POINT;
   }
   else
   {
      // Otherwise, we treat it as relative to the current directory
      // (for file creation) or FAT mount point (for directory creation)
      ccpt_nameStart = c_tempCharString;
      ccpt_basePath = b_createDirectory ? FAT_MOUNT_POINT : stpt_ctx->as8_currentDir;
   }

   // Ensure that the name part of the path is not empty
   if (ccpt_nameStart[0] == '\0')
   {
      return -EINVAL;
   }

   // Build the final path by concatenating the base path and the name part
   i_retVal = snprintf(cpt_builtPath, s_builtPathMaxSize, "%s/%s", ccpt_basePath, ccpt_nameStart);

   // Check if the resulting path fits in the output buffer and return appropriate status
   return (i_retVal > 0 && i_retVal < s_builtPathMaxSize) ? 0 : -ENAMETOOLONG;
}

/**
 * @private       sv_LogReadData
 * @brief         Helper function to print read data via logger module.
 * @param[in]     u8pt_data - Pointer to the data read.
 * @param[in]     u32_len - Length of the data read.
 * @param[out]    None
 * @param[inout]  None
 * @return        None
 */
static void sv_LogReadData(const uint8_t *u8pt_data, uint32_t u32_len)
{
   char as8_ascii[FS_MAX_CHUNK_SIZE + 1U];
   uint32_t u32_idx;

   if (u32_len > FS_MAX_CHUNK_SIZE)
   {
      u32_len = FS_MAX_CHUNK_SIZE;
   }

   for (u32_idx = 0U; u32_idx < u32_len; u32_idx++)
   {
      as8_ascii[u32_idx] = isprint(u8pt_data[u32_idx]) ? (char)u8pt_data[u32_idx] : '.';
   }
   as8_ascii[u32_len] = '\0';

   LOG_INF("Read %u bytes: %s", u32_len, as8_ascii);
}

static void sv_IdleEntry(void *vptr)
{
   ARG_UNUSED(vptr);
}

static enum smf_state_result se_IdleRun(void *vptr)
{
   FileSysManagerCTX_T *stpt_ctx = (FileSysManagerCTX_T *)vptr;

   switch (stpt_ctx->st_currentMsg.e_command)
   {
   case FSC_OPEN_DIR:
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_CREATE_DIR]);
      break;

   case FSC_OPEN_FILE:
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_CREATE_FILE]);
      break;

   case FSC_READ_FILE:
      LOG_WRN("Open a file first, then use read");
      break;

   case FSC_CLOSE_FILE:
   case FSC_ABORT:
      sv_CloseIfOpen(stpt_ctx);
      break;

   case FSC_WRITE_DATA:
   default:
      LOG_WRN("Unexpected command %d in IDLE", stpt_ctx->st_currentMsg.e_command);
      break;
   }

   return SMF_EVENT_HANDLED;
}

static void sv_CreateDIREntry(void *vptr)
{
   FileSysManagerCTX_T *stpt_ctx = (FileSysManagerCTX_T *)vptr;
   int i_retVal;

   i_retVal = si_BuildPathFromMsg(stpt_ctx, true, stpt_ctx->as8_currentDir, sizeof(stpt_ctx->as8_currentDir));
   if (i_retVal != 0)
   {
      LOG_ERR("OPEN_DIR path build failed (%d)", i_retVal);
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_FAILED]);
      return;
   }

   i_retVal = fs_mkdir(stpt_ctx->as8_currentDir);
   if ((i_retVal == 0) || (i_retVal == -EEXIST))
   {
      LOG_INF("Directory ready: %s", stpt_ctx->as8_currentDir);
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_IDLE]);
   }
   else
   {
      LOG_ERR("Directory create failed (%d): %s", i_retVal, stpt_ctx->as8_currentDir);
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_FAILED]);
   }
}

static enum smf_state_result se_CreateDIRRun(void *vptr)
{
   ARG_UNUSED(vptr);
   return SMF_EVENT_HANDLED;
}

static void sv_CreateFileEntry(void *vptr)
{
   FileSysManagerCTX_T *stpt_ctx = (FileSysManagerCTX_T *)vptr;
   int i_retVal;

   i_retVal = si_BuildPathFromMsg(stpt_ctx, false, stpt_ctx->as8_activeFile, sizeof(stpt_ctx->as8_activeFile));
   if (i_retVal != 0)
   {
      LOG_ERR("Unable to build path (%d): %s", i_retVal, stpt_ctx->as8_activeFile);
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_FAILED]);
   }

   sv_CloseIfOpen(stpt_ctx);
   fs_file_t_init(&stpt_ctx->file);

   i_retVal = fs_open(&stpt_ctx->file, stpt_ctx->as8_activeFile, FS_O_CREATE | FS_O_RDWR);
   if (i_retVal == 0)
   {
      stpt_ctx->b_fileOpenStatus = true;
      stpt_ctx->u32_byteWritten = 0U;
      LOG_INF("File opened: %s", stpt_ctx->as8_activeFile);
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_WRITE_FILE]);
   }
   else
   {
      LOG_ERR("File open failed (%d): %s", i_retVal, stpt_ctx->as8_activeFile);
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_FAILED]);
   }
}

static enum smf_state_result se_CreateFileRun(void *vptr)
{
   ARG_UNUSED(vptr);
   return SMF_EVENT_HANDLED;
}

static void sv_WriteFileEntry(void *vptr)
{
   ARG_UNUSED(vptr);
}

static enum smf_state_result se_WriteFileRun(void *vptr)
{
   FileSysManagerCTX_T *stpt_ctx = (FileSysManagerCTX_T *)vptr;
   int i_retVal;

   if (!stpt_ctx->b_fileOpenStatus)
   {
      LOG_ERR("WRITE state without open file");
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_FAILED]);
      return SMF_EVENT_HANDLED;
   }

   switch (stpt_ctx->st_currentMsg.e_command)
   {
   case FSC_WRITE_DATA:
      if ((stpt_ctx->st_currentMsg.u32_sizeOfData == 0U) ||
          (stpt_ctx->st_currentMsg.u32_sizeOfData > FS_MAX_CHUNK_SIZE))
      {
         LOG_ERR("Invalid write size: %u", stpt_ctx->st_currentMsg.u32_sizeOfData);
         smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_FAILED]);
         break;
      }

      i_retVal = fs_write(&stpt_ctx->file,
                       stpt_ctx->st_currentMsg.u8_data,
                       stpt_ctx->st_currentMsg.u32_sizeOfData);

      if (i_retVal < 0)
      {
         LOG_ERR("File write failed (%d)", i_retVal);
         smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_FAILED]);
         break;
      }

      stpt_ctx->u32_byteWritten += (uint32_t)i_retVal;
      LOG_INF("Written %d bytes, total %u", i_retVal, stpt_ctx->u32_byteWritten);
      break;

   case FSC_READ_FILE:
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_READ_FILE]);
      break;

   case FSC_CLOSE_FILE:
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_CLOSE]);
      break;

   case FSC_ABORT:
      sv_CloseIfOpen(stpt_ctx);
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_IDLE]);
      break;

   case FSC_OPEN_FILE:
   case FSC_OPEN_DIR:
   default:
      LOG_WRN("Unexpected command %d in WRITE", stpt_ctx->st_currentMsg.e_command);
      break;
   }

   return SMF_EVENT_HANDLED;
}

static void sv_ReadFileEntry(void *vptr)
{
   ARG_UNUSED(vptr);
}

static enum smf_state_result se_ReadFileRun(void *vptr)
{
   FileSysManagerCTX_T *stpt_ctx = (FileSysManagerCTX_T *)vptr;
   uint8_t au8_readBuf[FS_MAX_CHUNK_SIZE];
   uint32_t u32_lenToRead;
   int i_retVal;

   if (!stpt_ctx->b_fileOpenStatus)
   {
      LOG_ERR("READ state without open file");
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_FAILED]);
      return SMF_EVENT_HANDLED;
   }

   switch (stpt_ctx->st_currentMsg.e_command)
   {
   case FSC_READ_FILE:
      u32_lenToRead = stpt_ctx->st_currentMsg.u32_sizeOfData;
      if ((u32_lenToRead == 0U) || (u32_lenToRead > FS_MAX_CHUNK_SIZE))
      {
         u32_lenToRead = FS_MAX_CHUNK_SIZE;
      }

      i_retVal = fs_read(&stpt_ctx->file, au8_readBuf, u32_lenToRead);
      if (i_retVal < 0)
      {
         LOG_ERR("File read failed (%d)", i_retVal);
         smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_FAILED]);
         break;
      }

      if (i_retVal == 0)
      {
         LOG_INF("End of file reached: %s", stpt_ctx->as8_activeFile);
      }
      else
      {
         sv_LogReadData(au8_readBuf, (uint32_t)i_retVal);
      }
      break;

   case FSC_WRITE_DATA:
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_WRITE_FILE]);
      break;

   case FSC_CLOSE_FILE:
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_CLOSE]);
      break;

   case FSC_ABORT:
      sv_CloseIfOpen(stpt_ctx);
      smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_IDLE]);
      break;

   case FSC_OPEN_FILE:
   case FSC_OPEN_DIR:
   default:
      LOG_WRN("Unexpected command %d in READ", stpt_ctx->st_currentMsg.e_command);
      break;
   }

   return SMF_EVENT_HANDLED;
}

static void sv_CloseFileEntry(void *vptr)
{
   FileSysManagerCTX_T *stpt_ctx = (FileSysManagerCTX_T *)vptr;

   sv_CloseIfOpen(stpt_ctx);
   LOG_INF("File closed");
   smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_IDLE]);
}

static enum smf_state_result se_CloseFileRun(void *vptr)
{
   ARG_UNUSED(vptr);
   return SMF_EVENT_HANDLED;
}

static void sv_OpFailedEntry(void *vptr)
{
   FileSysManagerCTX_T *stpt_ctx = (FileSysManagerCTX_T *)vptr;

   sv_CloseIfOpen(stpt_ctx);
   LOG_ERR("FSM operation failed, returning to IDLE");
   smf_set_state(SMF_CTX(stpt_ctx), &state_table[STATE_IDLE]);
}

static enum smf_state_result se_OpFailedRun(void *vptr)
{
   ARG_UNUSED(vptr);
   return SMF_EVENT_HANDLED;
}

/******************************************************************************/
/*                        PUBLIC FUNCTION DEFINITIONS                         */
/******************************************************************************/
void gv_FileSysManagerFSMInit(FileSysManagerCTX_T *stpt_ctx)
{
   stpt_ctx->u32_byteWritten = 0U;
   stpt_ctx->u32_totalExpectedBytes = 0U;
   stpt_ctx->b_fileOpenStatus = false;
   (void)strncpy(stpt_ctx->as8_currentDir, FAT_MOUNT_POINT, sizeof(stpt_ctx->as8_currentDir) - 1U);
   stpt_ctx->as8_currentDir[sizeof(stpt_ctx->as8_currentDir) - 1U] = '\0';
   stpt_ctx->as8_activeFile[0] = '\0';

   smf_set_initial(SMF_CTX(stpt_ctx), &state_table[STATE_IDLE]);
}

void gv_FileSysManagerFSMRun(FileSysManagerCTX_T *stpt_ctx)
{
   smf_run_state(SMF_CTX(stpt_ctx));
}
