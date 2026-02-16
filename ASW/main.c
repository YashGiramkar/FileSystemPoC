#include <zephyr/fs/fs.h>
#include <ff.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

// Logging module for main.c
LOG_MODULE_REGISTER(main);

// Structure to hold FATFS and file information
static FATFS st_FatFs;
// File information structure for the file we will create and read from
struct fs_file_t st_FileInfo;

// Mount point and file paths
#define FAT_DRIVE_NAME "FLASH_DISK"
#define FAT_MOUNT_POINT "/" FAT_DRIVE_NAME ":"
#define VCU_DIR FAT_MOUNT_POINT "/VCU"
#define RTE_CONST_FILE VCU_DIR "/RTE_Const.txt"

// Sample Data string to write to the file
char c_DataArrayToRTE[] =
            "Hello from nRF54 FATFS! Writing this string to RTE Const File. "
            "Yes this is a test string to verify read/write functionality of "
            "FATFS on nRF54. And I believe that ****YES WE CAN!****";

// Buffer to read data back from the file, with space for null terminator
char c_ReadDataBuff[256];

// File system mount info structure
static struct fs_mount_t mount_info = {
    .type = FS_FATFS,
    .fs_data = &st_FatFs,
    .storage_dev = (void *)FAT_DRIVE_NAME,
    .mnt_point = FAT_MOUNT_POINT
};

int main(void)
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

    // Log that the FATFS has been successfully mounted
    LOG_INF("FATFS Mounted!");

    // Initialize the file information structure before using it
    fs_file_t_init(&st_FileInfo);

    // Create a directory for VCU if it doesn't exist, and handle errors
    i_RetCode = fs_mkdir(VCU_DIR);

    // If the directory creation fails with an error other than "already exists"
    if (i_RetCode < 0 && i_RetCode != -EEXIST)
    {
        LOG_ERR("Directory create failed (%d)", i_RetCode);
        return i_RetCode;
    }
    else
    {
        LOG_INF("Directory '%s' is ready", VCU_DIR);
    }

    // Open the RTE Const file for writing (create if it doesn't exist), and handle errors
    i_RetCode = fs_open(&st_FileInfo, RTE_CONST_FILE,
             FS_O_CREATE | FS_O_WRITE);

    // If the file open fails, log the error and exit
    if (i_RetCode < 0)
    {
        LOG_ERR("File open failed (%d)", i_RetCode);
        return i_RetCode;
    }
    else
    {
        LOG_INF("File '%s' opened for writing", RTE_CONST_FILE);
    }

    LOG_INF("Writing data to file '%s' : %s", RTE_CONST_FILE, c_DataArrayToRTE);

    // Write the sample data string to the RTE Const file, and handle errors
    i_RetCode = fs_write(&st_FileInfo, c_DataArrayToRTE, sizeof(c_DataArrayToRTE));
    if (i_RetCode < 0)
    {
        LOG_ERR("File write failed (%d)", i_RetCode);
        fs_close(&st_FileInfo);
        return i_RetCode;
    }
    else
    {
        LOG_INF("Data written to file '%s'", RTE_CONST_FILE);
    }
    // Close the RTE Const file after writing, and handle errors
    i_RetCode = fs_close(&st_FileInfo);
    if (i_RetCode < 0)
    {
        LOG_ERR("File close failed (%d)", i_RetCode);
        return i_RetCode;
    }
    else
    {
        LOG_INF("File '%s' closed after writing", RTE_CONST_FILE);
    }

    // Reopen the RTE Const file for reading
    i_RetCode = fs_open(&st_FileInfo, RTE_CONST_FILE, FS_O_READ);
    if (i_RetCode < 0)
    {
        LOG_ERR("File reopen failed (%d)", i_RetCode);
        return i_RetCode;
    }
    else
    {
        LOG_INF("File '%s' reopened for reading", RTE_CONST_FILE);
    }

    while (1)
    {
        // Read data from the file into the buffer
        i_RetCode = fs_read(&st_FileInfo, c_ReadDataBuff, sizeof(c_ReadDataBuff) - 1);
        // Handle errors from reading the file
        if (i_RetCode < 0)
        {
            LOG_ERR("File read failed (%d)", i_RetCode);
            fs_close(&st_FileInfo);
            return i_RetCode;
        }
        // If the read returns 0, it means we've reached the end of the file
        else if (i_RetCode == 0)
        {
            LOG_INF("End of file reached");
        }
        // If data was read successfully, log the number of bytes read
        else
        {
            LOG_INF("Read %d bytes from file '%s'", i_RetCode, RTE_CONST_FILE);
        }

        if (i_RetCode == 0)
        {
            break;
        }

        // If the read was successful and we got some data, null-terminate the buffer
        c_ReadDataBuff[i_RetCode] = '\0';
        // Log the data read from the file
        LOG_INF(" Read Data from %s : %s", RTE_CONST_FILE, c_ReadDataBuff);
    }

    // Close the RTE Const file after reading, and handle errors
    fs_close(&st_FileInfo);

    return 0;
}
