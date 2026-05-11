#include "../kernel_patches/CVE-2010-0036-HFS/hfs_validation.c"

static int failures;

static void check(int condition)
{
    if (!condition) {
        failures++;
    }
}

static void test_validate_hfs_extent_accepts_valid_extent(void)
{
    HFSExtentDescriptor extent = {
        .logicalSize = 4096,
        .totalBlocks = 8,
        .blockSize = 512,
    };

    check(validate_hfs_extent(&extent) == 1);
}

static void test_validate_hfs_extent_rejects_bad_sizes(void)
{
    HFSExtentDescriptor too_small_block = {
        .logicalSize = 512,
        .totalBlocks = 1,
        .blockSize = 256,
    };
    HFSExtentDescriptor too_large_block = {
        .logicalSize = 512,
        .totalBlocks = 1,
        .blockSize = 131072,
    };
    HFSExtentDescriptor overflowing_total = {
        .logicalSize = 4096,
        .totalBlocks = 0xFFFFFFFF,
        .blockSize = 4096,
    };
    HFSExtentDescriptor logical_too_large = {
        .logicalSize = 4097,
        .totalBlocks = 8,
        .blockSize = 512,
    };

    check(validate_hfs_extent(NULL) == 0);
    check(validate_hfs_extent(&too_small_block) == 0);
    check(validate_hfs_extent(&too_large_block) == 0);
    check(validate_hfs_extent(&overflowing_total) == 0);
    check(validate_hfs_extent(&logical_too_large) == 0);
}

static void test_validate_catalog_entry_accepts_valid_records(void)
{
    HFSCatalogFolder folder_record = {
        .recordType = 0x0001,
        .valence = 32767,
    };
    HFSCatalogFolder file_record = {
        .recordType = 0x0002,
        .valence = 0,
    };

    check(validate_catalog_entry(&folder_record) == 1);
    check(validate_catalog_entry(&file_record) == 1);
}

static void test_validate_catalog_entry_rejects_invalid_records(void)
{
    HFSCatalogFolder excessive_valence = {
        .recordType = 0x0001,
        .valence = 32768,
    };
    HFSCatalogFolder invalid_record_type = {
        .recordType = 0x0003,
        .valence = 1,
    };

    check(validate_catalog_entry(NULL) == 0);
    check(validate_catalog_entry(&excessive_valence) == 0);
    check(validate_catalog_entry(&invalid_record_type) == 0);
}

static void test_safe_catalog_lookup_bounds(void)
{
    check(safe_catalog_lookup(2, "System", 255) == 0);
    check(safe_catalog_lookup(1, "System", 6) == -1);
    check(safe_catalog_lookup(2, "System", 256) == -1);
}

int main(void)
{
    test_validate_hfs_extent_accepts_valid_extent();
    test_validate_hfs_extent_rejects_bad_sizes();
    test_validate_catalog_entry_accepts_valid_records();
    test_validate_catalog_entry_rejects_invalid_records();
    test_safe_catalog_lookup_bounds();

    return failures == 0 ? 0 : 1;
}
