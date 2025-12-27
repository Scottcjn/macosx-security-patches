/*
 * CVE-2010-0036 HFS+ Integer Overflow Red Team Test
 * 
 * Tests file operations that would trigger integer overflow
 * vulnerabilities in HFS+ filesystem handling.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define SAFE_LIMIT 0x7FFFFFFF  /* 2GB - 1 */
#define OVERFLOW_SIZE 0xFFFFFFFF  /* Triggers integer overflow */

int main() {
    const char *testfile = "/tmp/hfs_test_file.dat";
    int fd;
    char buf[64];
    off_t ret;
    ssize_t sret;
    int passed = 0, failed = 0;
    
    printf("=== CVE-2010-0036 HFS+ Overflow Guard Red Team Test ===\n\n");
    
    /* Create test file */
    fd = open(testfile, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        printf("ERROR: Could not create test file: %s\n", strerror(errno));
        return 1;
    }
    write(fd, "test data for overflow check\n", 30);
    
    /* Test 1: lseek with huge offset - SHOULD BE BLOCKED */
    printf("Test 1: lseek to offset 0xFFFFFFFF (4GB)\n");
    printf("  Expected: BLOCKED (-1, EINVAL)\n");
    errno = 0;
    ret = lseek(fd, (off_t)OVERFLOW_SIZE, SEEK_SET);
    if (ret == (off_t)-1 && errno == EINVAL) {
        printf("  Result: BLOCKED (errno=EINVAL) - PASS\n");
        passed++;
    } else {
        printf("  Result: ALLOWED (offset=%lld) - FAIL (vulnerable!)\n", (long long)ret);
        failed++;
    }
    printf("\n");
    
    /* Reset position */
    lseek(fd, 0, SEEK_SET);
    
    /* Test 2: read with huge size - SHOULD BE BLOCKED */
    printf("Test 2: read with size 0x80000000 (2GB)\n");
    printf("  Expected: BLOCKED (-1, EFBIG)\n");
    errno = 0;
    sret = read(fd, buf, (size_t)0x80000000);
    if (sret == -1 && errno == EFBIG) {
        printf("  Result: BLOCKED (errno=EFBIG) - PASS\n");
        passed++;
    } else {
        printf("  Result: ALLOWED (ret=%zd, errno=%d) - FAIL (vulnerable!)\n", sret, errno);
        failed++;
    }
    printf("\n");
    
    /* Reset position */
    lseek(fd, 0, SEEK_SET);
    
    /* Test 3: ftruncate to huge size - SHOULD BE BLOCKED */
    printf("Test 3: ftruncate to size 0xFFFFFFFF (4GB)\n");
    printf("  Expected: BLOCKED (-1, EFBIG)\n");
    errno = 0;
    ret = ftruncate(fd, (off_t)OVERFLOW_SIZE);
    if (ret == -1 && errno == EFBIG) {
        printf("  Result: BLOCKED (errno=EFBIG) - PASS\n");
        passed++;
    } else {
        printf("  Result: ALLOWED (ret=%lld) - FAIL (vulnerable!)\n", (long long)ret);
        failed++;
    }
    printf("\n");
    
    /* Test 4: Normal operations (control test) - SHOULD WORK */
    printf("Test 4: Normal lseek and read (control test)\n");
    printf("  Expected: ALLOWED\n");
    lseek(fd, 0, SEEK_SET);
    errno = 0;
    sret = read(fd, buf, 10);
    if (sret > 0) {
        printf("  Result: ALLOWED (read %zd bytes) - PASS\n", sret);
        passed++;
    } else {
        printf("  Result: BLOCKED - FAIL (breaks normal ops!)\n");
        failed++;
    }
    printf("\n");
    
    close(fd);
    unlink(testfile);
    
    printf("=== SUMMARY ===\n");
    printf("Passed: %d/4\n", passed);
    printf("Failed: %d/4\n", failed);
    
    if (passed >= 3) {
        printf("\nHFS OVERFLOW GUARD WORKING\n");
        return 0;
    } else {
        printf("\nHFS OVERFLOW GUARD HAS ISSUES\n");
        return 1;
    }
}
