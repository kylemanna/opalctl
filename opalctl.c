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
#include <stdint.h>

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

int main(int argc, char* argv[])
{
    // Unused variables
    (void) argc;
    (void) argv;

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

    // Create necessary structure and zerofill it, just in case
    struct opal_lock_unlock lk_unlk;
    memset(&lk_unlk, 0, sizeof(struct opal_lock_unlock));
    
    // Unlock OPAL drive for read and write
    lk_unlk.l_state = OPAL_RW;
    // Don't use single user mode
    lk_unlk.session.sum = 0;
    // Identify as admin1
    lk_unlk.session.who = OPAL_ADMIN1;
    // 0 locking range
    lk_unlk.session.opal_key.lr = 0;
    // Copy key
    memcpy(lk_unlk.session.opal_key.key, hash, sizeof(hash));
    // Set key size
    lk_unlk.session.opal_key.key_len = sizeof(hash);
    
    // Check if everything is OK now
    if (errno) {
        perror("Error before ioctl");
        goto cleanup;
    }
    
    // Test the lk_unlk structure, this will give an error when the password is incorrect
    ret = ioctl(fd, IOC_OPAL_LOCK_UNLOCK, &lk_unlk);
    if (ret != 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Failed to ioctl(%s, IOC_OPAL_LOCK_UNLOCK, ...)", dev);
        perror(buf);
        goto cleanup;
    }

    // Do the actual job
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
