#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "FileSysManager.h"
#include "TransferMsgTypes.h"

LOG_MODULE_REGISTER(MAIN);

#define UART_LINE_BUF_SIZE                   256

static const struct device *stpt_consoleUART = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static void sv_PrintHelp(void)
{
   printk("\nFileSys UART Test Commands:\n");
   printk("  help\n");
   printk("  mkdir <path>\n");
   printk("  open <path>\n");
   printk("  write <text payload>\n");
   printk("  read [bytes]\n");
   printk("  close\n");
   printk("  abort\n\n");
}

static int si_UartReadLine(char *cptr_rxLine, uint8_t u8_maxLen)
{
      uint8_t u8_idx = 0U;
      unsigned char uc_readChar;

   while (u8_idx < (u8_maxLen - 1U))
   {
      if (uart_poll_in(stpt_consoleUART, &uc_readChar) == 0)
      {
         if ((uc_readChar == '\r') || (uc_readChar == '\n')) {
               cptr_rxLine[u8_idx] = '\0';
               printk("\r\n");
               return (int)u8_idx;
         }

         if ((uc_readChar == '\b') || (uc_readChar == 127U))
         {
            if (u8_idx > 0U)
            {
               u8_idx--;
               printk("\b \b");
            }
            continue;
         }

         cptr_rxLine[u8_idx++] = (char)uc_readChar;
         uart_poll_out(stpt_consoleUART, uc_readChar);
      }
      else
      {
         k_msleep(10);
      }
   }

   cptr_rxLine[u8_maxLen - 1U] = '\0';
   return (int)(u8_maxLen - 1U);
}

static char *sp_ArgStart(char *cptr_rxLine)
{
   char *arg = strchr(cptr_rxLine, ' ');

   if (arg == NULL)
   {
      return NULL;
   }

   *arg = '\0';
   arg++;

   while (*arg == ' ')
   {
      arg++;
   }

   return (*arg == '\0') ? NULL : arg;
}

static int si_EnqueueMsg(const FileSysMessage_T *msg)
{
   struct k_msgq *FSMGR_MsgQ = gstpt_FSMGR_GetMsgQ();

   return k_msgq_put(FSMGR_MsgQ, msg, K_MSEC(100));
}

static int si_ParseHexByte(const char *token, uint8_t *out)
{
   char *end = NULL;
   unsigned long value = strtoul(token, &end, 16);

   if ((token[0] == '\0') || (*end != '\0') || (value > 0xFFUL))
   {
      return -EINVAL;
   }

   *out = (uint8_t)value;
   return 0;
}

static void sv_HandleLine(char *cptr_rxLine)
{
   FileSysMessage_T msg;
   char *arg;
   char *end;
   unsigned long ul_readSize;
   int ret;

   while (isspace((unsigned char)*cptr_rxLine))
   {
      cptr_rxLine++;
   }

   if (*cptr_rxLine == '\0')
   {
      return;
   }

   arg = sp_ArgStart(cptr_rxLine);

   (void)memset(&msg, 0, sizeof(msg));

   if (strcmp(cptr_rxLine, "help") == 0)
   {
      sv_PrintHelp();
      return;
   }

   if (strcmp(cptr_rxLine, "mkdir") == 0)
   {
      if (arg == NULL)
      {
         printk("mkdir requires a path\n");
         return;
      }

      msg.e_command = FSC_OPEN_DIR;
      msg.u32_sizeOfData = MIN((uint32_t)strlen(arg), (uint32_t)FS_MAX_CHUNK_SIZE);
      (void)memcpy(msg.u8_data, arg, msg.u32_sizeOfData);
   }
   else if (strcmp(cptr_rxLine, "open") == 0)
   {
      if (arg == NULL)
      {
         printk("open requires a path\n");
         return;
      }

      msg.e_command = FSC_OPEN_FILE;
      msg.u32_sizeOfData = MIN((uint32_t)strlen(arg), (uint32_t)FS_MAX_CHUNK_SIZE);
      (void)memcpy(msg.u8_data, arg, msg.u32_sizeOfData);
   }
   else if (strcmp(cptr_rxLine, "write") == 0)
   {
      if (arg == NULL)
      {
         printk("write requires payload\n");
         return;
      }

      msg.e_command = FSC_WRITE_DATA;
      msg.u32_sizeOfData = MIN((uint32_t)strlen(arg), (uint32_t)FS_MAX_CHUNK_SIZE);
      (void)memcpy(msg.u8_data, arg, msg.u32_sizeOfData);
   }
   else if (strcmp(cptr_rxLine, "read") == 0)
   {
      msg.e_command = FSC_READ_FILE;
      msg.u32_sizeOfData = FS_MAX_CHUNK_SIZE;

      if (arg != NULL)
      {
         ul_readSize = strtoul(arg, &end, 10);
         if ((*arg == '\0') || (*end != '\0') || (ul_readSize == 0UL))
         {
            printk("read expects optional positive byte count\n");
            return;
         }

         msg.u32_sizeOfData = MIN((uint32_t)ul_readSize, (uint32_t)FS_MAX_CHUNK_SIZE);
      }
   }
   else if (strcmp(cptr_rxLine, "close") == 0)
   {
      msg.e_command = FSC_CLOSE_FILE;
   }
   else if (strcmp(cptr_rxLine, "abort") == 0)
   {
      msg.e_command = FSC_ABORT;
   }
   else
   {
      printk("Unknown command: %s\n", cptr_rxLine);
      sv_PrintHelp();
      return;
   }

   ret = si_EnqueueMsg(&msg);
   if (ret != 0)
   {
      printk("Queue put failed (%d)\n", ret);
      return;
   }

   printk("Queued cmd=%d size=%u\n", msg.e_command, msg.u32_sizeOfData);
}

int main(void)
{
   char cptr_rxLine[UART_LINE_BUF_SIZE];

   if (!device_is_ready(stpt_consoleUART))
   {
      LOG_ERR("Console UART device is not ready");
      return -ENODEV;
   }

   printk("\nFileSysManager UART harness ready. Type 'help'.\n> ");

   while (1)
   {
      int len = si_UartReadLine(cptr_rxLine, sizeof(cptr_rxLine));

      if (len > 0)
      {
         sv_HandleLine(cptr_rxLine);
      }

      printk("> ");
   }

   return 0;
}
