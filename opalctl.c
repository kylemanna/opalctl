/**
 * Front end to Linux Kernel SED TCG OPAL userpace interface
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/sed-opal.h>

/*
 * Get this out of GDB from the sedutil-cli gdb function
$2 = std::vector of length 34, capacity 34 = {0xd0, 0x20, ... }
 * First 2 chars are token and length, remove and insert rest below:
 */

char hash[OPAL_KEY_MAX] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

int main(char* argv[], int argc)
{
    int ret = 0;

    char *dev = "/dev/nvme0n1";
    int fd = open(dev, O_WRONLY);
    if (fd < 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Failed to open %s", dev);
        perror(buf);
        ret = -1;
        goto exit;
    }

    /* Rough notes about kernel call graph:

    struct opal_lock_unlock {
        struct opal_session_info session;
        __u32 l_state;
        __u8 __align[4];
    };

    Sets:

    suspend->unlk = *lk_unlk;
    suspend->lr = lk_unlk->session.opal_key.lr;

    __opal_lock_unlock(..., struct opal_lock_unlock *lk_unlk) {
        next();
    }

    SUM = Single User Mode:

    lock_unlock_locking_range() or lock_unlock_locking_range_sum()

    */

    struct opal_lock_unlock lk_unlk = {
        .session = {
            .sum = 0, /* Not Single User Mode */
            .who = OPAL_ADMIN1, /* Act as admin */
            .opal_key = {
                .lr = 0, /* Locking Range = 0 -> global */
            },
        },
        .l_state = OPAL_RW, /* Allow read-write, but not sure */
    };

    //const char *prompt = "TCG Opal Password: ";
    //char* pass = getpass(prompt);

    //size_t len = strlen(pass);
    //memcpy(&lk_unlk.session.opal_key.key, pass, len);
    memcpy(&lk_unlk.session.opal_key.key, hash, OPAL_KEY_MAX);

    printf("sizeof(lk_unlk) = %u\n", sizeof(lk_unlk));
    ret = ioctl(fd, IOC_OPAL_SAVE, &lk_unlk);

    if (ret != 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Failed to ioctl(%s, IOC_OPAL_SAVE, ...)", dev);
        perror(buf);
        goto cleanup;
    }

cleanup:
    close(fd);
exit:
    return ret;
}
