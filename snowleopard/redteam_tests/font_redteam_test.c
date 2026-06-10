/*
 * CVE-2011-0182 Font Parsing Red Team Test
 * 
 * This test creates malformed font files that would trigger
 * the vulnerability and verifies the guard blocks them.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

/* Write big-endian 32-bit */
void write_be32(uint8_t *p, uint32_t v) {
    p[0] = (v >> 24) & 0xFF;
    p[1] = (v >> 16) & 0xFF;
    p[2] = (v >> 8) & 0xFF;
    p[3] = v & 0xFF;
}

/* Write big-endian 16-bit */
void write_be16(uint8_t *p, uint16_t v) {
    p[0] = (v >> 8) & 0xFF;
    p[1] = v & 0xFF;
}

/* Test 1: Font with table offset beyond file (CVE trigger) */
int create_overflow_font(const char *path) {
    uint8_t font[128];
    int fd;
    
    memset(font, 0, sizeof(font));
    
    /* TrueType magic */
    write_be32(font, 0x00010000);
    
    /* numTables = 1 */
    write_be16(font + 4, 1);
    
    /* searchRange, entrySelector, rangeShift */
    write_be16(font + 6, 16);
    write_be16(font + 8, 0);
    write_be16(font + 10, 16);
    
    /* Table directory entry at offset 12 */
    font[12] = 'g'; font[13] = 'l'; font[14] = 'y'; font[15] = 'f';  /* tag */
    write_be32(font + 16, 0);           /* checksum */
    write_be32(font + 20, 0x00FFFFFF);  /* offset WAY beyond file - OVERFLOW TRIGGER */
    write_be32(font + 24, 0xFFFFFFFF);  /* length - integer overflow trigger */
    
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;
    write(fd, font, 64);  /* Small file with huge offset claim */
    close(fd);
    return 0;
}

/* Test 2: Font with too many tables */
int create_too_many_tables_font(const char *path) {
    uint8_t font[32];
    int fd;
    
    memset(font, 0, sizeof(font));
    
    /* TrueType magic */
    write_be32(font, 0x00010000);
    
    /* numTables = 500 (> 256 max) */
    write_be16(font + 4, 500);
    
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;
    write(fd, font, sizeof(font));
    close(fd);
    return 0;
}

/* Test 3: Invalid magic number */
int create_invalid_magic_font(const char *path) {
    uint8_t font[32];
    int fd;
    
    memset(font, 0, sizeof(font));
    
    /* Invalid magic - not TTF/OTF/TTC */
    write_be32(font, 0xDEADBEEF);
    write_be16(font + 4, 1);
    
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;
    write(fd, font, sizeof(font));
    close(fd);
    return 0;
}

/* Test 4: Valid-looking font (should be allowed) */
int create_valid_font(const char *path) {
    uint8_t font[128];
    int fd;
    
    memset(font, 0, sizeof(font));
    
    /* TrueType magic */
    write_be32(font, 0x00010000);
    
    /* numTables = 1 */
    write_be16(font + 4, 1);
    write_be16(font + 6, 16);
    write_be16(font + 8, 0);
    write_be16(font + 10, 16);
    
    /* Valid table entry */
    font[12] = 'n'; font[13] = 'a'; font[14] = 'm'; font[15] = 'e';
    write_be32(font + 16, 0);   /* checksum */
    write_be32(font + 20, 28);  /* offset within file */
    write_be32(font + 24, 32);  /* reasonable length */
    
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;
    write(fd, font, 64);
    close(fd);
    return 0;
}

int try_open_font(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd >= 0) {
        close(fd);
        return 1;  /* Opened successfully */
    }
    return 0;  /* Blocked or error */
}

int main() {
    const char *test1 = "/tmp/test_overflow.ttf";
    const char *test2 = "/tmp/test_too_many_tables.ttf";
    const char *test3 = "/tmp/test_invalid_magic.ttf";
    const char *test4 = "/tmp/test_valid.ttf";
    
    int passed = 0, failed = 0;
    
    printf("=== CVE-2011-0182 Font Guard Red Team Test ===\n\n");
    
    /* Create test fonts */
    create_overflow_font(test1);
    create_too_many_tables_font(test2);
    create_invalid_magic_font(test3);
    create_valid_font(test4);
    
    /* Test 1: Overflow offset - SHOULD BE BLOCKED */
    printf("Test 1: Font with table offset overflow (0x00FFFFFF)\n");
    printf("  Expected: BLOCKED (file not found)\n");
    if (try_open_font(test1)) {
        printf("  Result: OPENED - FAIL (vulnerable!)\n");
        failed++;
    } else {
        printf("  Result: BLOCKED - PASS (mitigated)\n");
        passed++;
    }
    printf("\n");
    
    /* Test 2: Too many tables - SHOULD BE BLOCKED */
    printf("Test 2: Font with 500 tables (max 256)\n");
    printf("  Expected: BLOCKED\n");
    if (try_open_font(test2)) {
        printf("  Result: OPENED - FAIL (vulnerable!)\n");
        failed++;
    } else {
        printf("  Result: BLOCKED - PASS (mitigated)\n");
        passed++;
    }
    printf("\n");
    
    /* Test 3: Invalid magic - SHOULD BE BLOCKED */
    printf("Test 3: Font with invalid magic (0xDEADBEEF)\n");
    printf("  Expected: BLOCKED\n");
    if (try_open_font(test3)) {
        printf("  Result: OPENED - FAIL (vulnerable!)\n");
        failed++;
    } else {
        printf("  Result: BLOCKED - PASS (mitigated)\n");
        passed++;
    }
    printf("\n");
    
    /* Test 4: Valid font - SHOULD BE ALLOWED */
    printf("Test 4: Valid-looking font (control test)\n");
    printf("  Expected: ALLOWED\n");
    if (try_open_font(test4)) {
        printf("  Result: OPENED - PASS (valid fonts work)\n");
        passed++;
    } else {
        printf("  Result: BLOCKED - FAIL (breaks valid fonts!)\n");
        failed++;
    }
    printf("\n");
    
    printf("=== SUMMARY ===\n");
    printf("Passed: %d/4\n", passed);
    printf("Failed: %d/4\n", failed);
    
    if (failed == 0) {
        printf("\nFONT GUARD WORKING CORRECTLY\n");
        return 0;
    } else {
        printf("\nFONT GUARD HAS ISSUES\n");
        return 1;
    }
}
