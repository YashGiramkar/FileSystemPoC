#include <zephyr/fs/fs.h>
#include <ff.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(main);

static FATFS fat_fs;
struct fs_file_t file;

char data[] = "Hello from nRF54 FATFS!\n";
char read_buf[32];

static struct fs_mount_t mount_info = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
    .storage_dev = (void *)"FLASH_DISK",
    .mnt_point = "/FLASH:"
};

int main(void)
{
    int rc;

    rc = fs_mount(&mount_info);
    if (rc < 0) {
        LOG_INF("Mount failed (%d), formatting...", rc);

        rc = fs_mkfs(FS_FATFS, (uintptr_t)"FLASH_DISK", NULL, 0);
        if (rc < 0) {
            LOG_ERR("Format failed (%d)", rc);
            return rc;
        }

        rc = fs_mount(&mount_info);
        if (rc < 0) {
            LOG_ERR("Mount failed after format (%d)", rc);
            return rc;
        }
    }

    LOG_INF("FATFS Mounted!");

    fs_file_t_init(&file);

    rc = fs_open(&file, "/FLASH:/test.txt",
             FS_O_CREATE | FS_O_WRITE);

    if (rc < 0)
    {
        LOG_ERR("File open failed");
        return 0;
    }

    rc = fs_write(&file, data, sizeof(data));
    if (rc < 0) {
        LOG_ERR("File write failed (%d)", rc);
        fs_close(&file);
        return rc;
    }

    rc = fs_close(&file);
    if (rc < 0) {
        LOG_ERR("File close failed (%d)", rc);
        return rc;
    }

    rc = fs_open(&file, "/FLASH:/test.txt", FS_O_READ);
    if (rc < 0) {
        LOG_ERR("File reopen failed (%d)", rc);
        return rc;
    }

    while (1) {
        rc = fs_read(&file, read_buf, sizeof(read_buf) - 1);
        if (rc < 0) {
            LOG_ERR("File read failed (%d)", rc);
            fs_close(&file);
            return rc;
        }

        if (rc == 0) {
            break;
        }

        read_buf[rc] = '\0';
        printk("%s", read_buf);
    }

    fs_close(&file);

    return 0;
}
