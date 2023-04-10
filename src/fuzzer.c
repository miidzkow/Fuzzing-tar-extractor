#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>


// Header structure
struct tar_t
{                              /* byte offset */
    char name[100];               /*   0 */
    char mode[8];                 /* 100 */
    char uid[8];                  /* 108 */
    char gid[8];                  /* 116 */
    char size[12];                /* 124 */
    char mtime[12];               /* 136 */
    char chksum[8];               /* 148 */
    char typeflag[1];             /* 156 */
    char linkname[100];           /* 157 */
    char magic[6];                /* 257 */
    char version[2];              /* 263 */
    char uname[32];               /* 265 */
    char gname[32];               /* 297 */
    char devmajor[8];             /* 329 */
    char devminor[8];             /* 337 */
    char prefix[155];             /* 345 */
    char padding[12];             /* 500 */
};

/**
 * Computes the checksum for a tar header and encode it on the header
 * @param entry: The tar header
 * @return the value of the checksum
 */
unsigned int calculate_checksum(struct tar_t* entry) {
    // use spaces for the checksum bytes while calculating the checksum
    memset(entry->chksum, ' ', 8);

    // sum of entire metadata
    unsigned int check = 0;
    unsigned char* raw = (unsigned char*) entry;
    for(int i = 0; i < 512; i++){
        check += raw[i];
    }

    // Checksum is terminated by a null character and a space character (0x00 0x20)
    snprintf(entry->chksum, sizeof(entry->chksum), "%06o0", check);

    entry->chksum[6] = '\0';
    entry->chksum[7] = ' ';
    return check;
}


/**
 * Generates a tar header given the inputed values
 * @param header
 * @param name
 * @param mode
 * @param uid
 * @param gid
 * @param size
 * @param mtime
 * @param typeflag
 * @param linkname
 * @param magic
 * @param version
 * @param uname
 * @param gname
 */
void generate_tar_header(struct tar_t *header, const char *name, const char *mode, const char *uid, const char *gid, const char *size,
                         const char *mtime, const char *typeflag, const char *linkname, const char *magic, const char *version,
                         const char *uname, const char *gname) {
    // Copy the input parameters into the tar header fields
    strncpy(header->name, name, sizeof(header->name));
    strncpy(header->mode, mode, sizeof(header->mode));
    strncpy(header->uid, uid, sizeof(header->uid));
    strncpy(header->gid, gid, sizeof(header->gid));
    strncpy(header->size, size, sizeof(header->size));
    strncpy(header->mtime, mtime, sizeof(header->mtime));
    strncpy(header->typeflag, typeflag, sizeof(header->typeflag));
    strncpy(header->linkname, linkname, sizeof(header->linkname));
    strncpy(header->magic, magic, sizeof(header->magic));
    strncpy(header->version, version, sizeof(header->version));
    strncpy(header->uname, uname, sizeof(header->uname));
    strncpy(header->gname, gname, sizeof(header->gname));
    strncpy(header->devmajor, "0000000", sizeof(header->devmajor));
    strncpy(header->devminor, "0000000", sizeof(header->devminor));
    strncpy(header->prefix, "", sizeof(header->prefix));
    strncpy(header->padding, "", sizeof(header->padding));
}

/**
 * Creates a .tar archive
 * @param filename name of the archive to be created
 * @param header header data to be added in the tar archive
 */
void write_tar_file(const char* filename, struct tar_t* header) {
    // Open file for writing in binary mode
    FILE* file = fopen(filename, "wb");

    // Write the header to the file
    fwrite(header, sizeof(struct tar_t), 1, file);

    // Close the file
    fclose(file);
}

/**
 * Creates a .tar archive of a file with data and padding
 * @param filename name of the archive to be created
 * @param header header data to be added in the tar archive
 * @param data data that has to be added
 * @param data_len data size used to calculate the padding
 */
void write_tar_file_with_data(const char* filename, struct tar_t* header, const char* data, int data_len) {
    // Open file for writing in binary mode
    FILE* file = fopen(filename, "wb");

    // Write the header to the file
    fwrite(header, sizeof(struct tar_t), 1, file);

    // Write the data to the file
    fwrite(data, sizeof(char), data_len, file);

    // Pad the data with null bytes (it has to be a 512-byte block)
    int padding_len = 512 - (data_len % 512);
    if (padding_len != 512) {
        char padding[padding_len];
        memset(padding, 0, padding_len);
        fwrite(padding, sizeof(char), padding_len, file);
    }

    // Close the file
    fclose(file);
}


/**
 * Function that calls the external extractor with the file to be extracted
 * @param extractor the extractor that will be used
 * @param filename name of the tar archive to be extracted
 * @return -1 if the executable cannot be launched,
 *          0 if it is launched but does not crash,
 *          1 if it is launched and it crashed.
 */
int extract(char* extractor, char * filename) {
    int rv = 0;
    char cmd[51];
    strncpy(cmd, extractor, 25);
    cmd[26] = '\0';
    strncat(cmd, filename, 25);
    char buf[33];
    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return -1;
    }

    if(fgets(buf, 33, fp) == NULL) {
        //printf("No output\n");
        goto finally;
    }
    if(strncmp(buf, "*** The program has crashed ***\n", 33)) {
        //printf("Not the crash message\n");
        goto finally;
    } else {
        //printf("Crash message\n");
        rv = 1;
        goto finally;
    }
    finally:
    if(pclose(fp) == -1) {
        printf("Command not found\n");
        rv = -1;
    }
    return rv;
}


/**
 * Tests tar with name field with all non-ascii characters at each position (one position by one, not all combinations)
 * File without data
 * Single file in archive
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_filename_field(char* extractor) {

    printf("Testing the field 'name' with non-ascii characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char filename[99];
    // By default, the filename is 99 times "a" as the filename has to be null-terminated
    memset(filename, 'a', sizeof(filename));

    struct tar_t header;

    // Loop through each position in the filename and replace by a character from 0x00 to 0xFF
    for (i = 0; i < 99; i++) {
        for (j = 0x00; j <= 0xFF; j++) {

            // Skip the ascii characters
            if (j >= 0x20 && j <= 0x7F) {
                continue;
            }

            filename[i] = (char) j;

            // Generate a header with other fields that are correct, only manipulate the filename field
            generate_tar_header(&header, filename, "0000664", "0001750", "0001750", "00000000062",
                                "14413537165", "0", "", "ustar", "00", "michal", "michal");

            calculate_checksum(&header);


            write_tar_file("test_filename.tar", &header);


            if (extract(extractor, " test_filename.tar") == 1 ) {
                // The extractor has crashed
                rename("test_filename.tar", "success_filename.tar");
                // Delete the extracted file
                remove(filename);
                // return 1 to stop the execution as one crash is enough
                return 1;
            } else {
                // Delete the extracted file
                remove(filename);
                // Keep going, maybe next character or next position will make it crash
            }
        }
        filename[i] = 'a';
    }

    remove("test_filename.tar");
    return 0;
}


/**
 * Tests tar with mode field with all characters at each position (one position by one, not all combinations)
 * File without data
 * Single file in archive
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_mode_field(char* extractor) {
    printf("Testing the field 'mode' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char mode[7];
    // By default, the mode is 7 times "0" as the mode has to be null-terminated
    memset(mode, '0', sizeof(mode));

    struct tar_t header;

    // Loop through each position in the mode and replace by a character from 0x00 to 0xFF
    for (i = 0; i < 7; i++) {
        for (j = 0x00; j <= 0xFF; j++) {

            mode[i] = (char) j;

            // Generate a header with other fields that are correct, only manipulate the mode field
            generate_tar_header(&header, "file.txt", mode, "0001750", "0001750", "00000000062",
                                "14413537165", "0", "", "ustar", "00", "michal", "michal");

            calculate_checksum(&header);


            write_tar_file("test_mode.tar", &header);


            if (extract(extractor, " test_mode.tar") == 1 ) {
                // The extractor has crashed
                rename("test_mode.tar", "success_mode.tar");
                // Delete the extracted file
                remove("file.txt");
                // return 1 to stop the execution as one crash is enough
                return 1;
            } else {
                // Delete the extracted file
                remove("file.txt");
                // Keep going, maybe next character or next position will make it crash
            }
        }
        mode[i] = '0';
    }

    remove("test_mode.tar");
    return 0;
}


/**
 * Tests tar with uid field with all characters at each position (one position by one, not all combinations)
 * File without data
 * Single file in archive
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_uid_field(char* extractor) {
    printf("Testing the field 'uid' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char uid[7];
    // By default, the uid is 7 times "0" as the uid has to be null-terminated
    memset(uid, '0', sizeof(uid));

    struct tar_t header;

    // Loop through each position in the mode and replace by a character from 0x00 to 0xFF
    for (i = 0; i < 7; i++) {
        for (j = 0x00; j <= 0xFF; j++) {

            uid[i] = (char) j;

            // Generate a header with other fields that are correct, only manipulate the mode field
            generate_tar_header(&header, "file.txt", "0000664", uid, "0001750", "00000000062",
                                "14413537165", "0", "", "ustar", "00", "michal", "michal");

            calculate_checksum(&header);


            write_tar_file("test_uid.tar", &header);


            if (extract(extractor, " test_uid.tar") == 1 ) {
                // The extractor has crashed
                rename("test_uid.tar", "success_uid.tar");
                // Delete the extracted file
                remove("file.txt");
                // return 1 to stop the execution as one crash is enough
                return 1;
            } else {
                // Delete the extracted file
                remove("file.txt");
                // Keep going, maybe next character or next position will make it crash
            }
        }
        uid[i] = '0';
    }

    remove("test_uid.tar");
    return 0;
}


/**
 * Tests tar with gid field all characters at each position (one position by one, not all combinations)
 * File without data
 * Single file in archive
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_gid_field(char* extractor) {
    printf("Testing the field 'gid' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char gid[7];
    // By default, the gid is 7 times "0" as the gid has to be null-terminated
    memset(gid, '0', sizeof(gid));

    struct tar_t header;

    // Loop through each position in the mode and replace by a character from 0x00 to 0xFF
    for (i = 0; i < 7; i++) {
        for (j = 0x00; j <= 0xFF; j++) {

            gid[i] = (char) j;

            // Generate a header with other fields that are correct, only manipulate the mode field
            generate_tar_header(&header, "file.txt", "0000664", "0001750", gid, "00000000062",
                                "14413537165", "0", "", "ustar", "00", "michal", "michal");

            calculate_checksum(&header);


            write_tar_file("test_gid.tar", &header);


            if (extract(extractor, " test_gid.tar") == 1 ) {
                // The extractor has crashed
                rename("test_gid.tar", "success_gid.tar");
                // Delete the extracted file
                remove("file.txt");
                // return 1 to stop the execution as one crash is enough
                return 1;
            } else {
                // Delete the extracted file
                remove("file.txt");
                // Keep going, maybe next character or next position will make it crash
            }
        }
        gid[i] = '0';
    }

    remove("test_gid.tar");
    return 0;
}


/**
 * Tests tar with size field with all characters at each position (one position by one, not all combinations)
 * File without data
 * Single file in archive
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_size_field(char* extractor) {
    printf("Testing the field 'size' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char size[11];
    // By default, the size is 11 times "0" as the size has to be null-terminated
    memset(size, '0', sizeof(size));

    struct tar_t header;

    // Loop through each position in the mode and replace by a character from 0x00 to 0xFF
    for (i = 0; i < 11; i++) {
        for (j = 0x00; j <= 0xFF; j++) {

            size[i] = (char) j;

            // Generate a header with other fields that are correct, only manipulate the mode field
            generate_tar_header(&header, "file.txt", "0000664", "0001750", "0001750", size,
                                "14413537165", "0", "", "ustar", "00", "michal", "michal");

            calculate_checksum(&header);


            write_tar_file("test_size.tar", &header);


            if (extract(extractor, " test_size.tar") == 1 ) {
                // The extractor has crashed
                rename("test_size.tar", "success_size.tar");
                // Delete the extracted file
                remove("file.txt");
                // return 1 to stop the execution as one crash is enough
                return 1;
            } else {
                // Delete the extracted file
                remove("file.txt");
                // Keep going, maybe next character or next position will make it crash
            }
        }
        size[i] = '0';
    }

    remove("test_size.tar");
    return 0;
}


/**
 * Tests tar with mtime field with all characters at each position (one position by one, not all combinations)
 * File without data
 * Single file in archive
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_mtime_field(char* extractor) {
    printf("Testing the field 'mtime' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char mtime[11];
    // By default, the mtime is 11 times "0" as the mtime has to be null-terminated
    memset(mtime, '0', sizeof(mtime));

    struct tar_t header;

    // Loop through each position in the mode and replace by a character from 0x00 to 0xFF
    for (i = 0; i < 11; i++) {
        for (j = 0x00; j <= 0xFF; j++) {

            mtime[i] = (char) j;

            // Generate a header with other fields that are correct, only manipulate the mode field
            generate_tar_header(&header, "file.txt", "0000664", "0001750", "0001750", "00000000062",
                                mtime, "0", "", "ustar", "00", "michal", "michal");

            calculate_checksum(&header);


            write_tar_file("test_mtime.tar", &header);


            if (extract(extractor, " test_mtime.tar") == 1 ) {
                // The extractor has crashed
                rename("test_mtime.tar", "success_mtime.tar");
                // Delete the extracted file
                remove("file.txt");
                // return 1 to stop the execution as one crash is enough
                return 1;
            } else {
                // Delete the extracted file
                remove("file.txt");
                // Keep going, maybe next character or next position will make it crash
            }
        }
        mtime[i] = '0';
    }

    remove("test_mtime.tar");
    return 0;
}


/**
 * Tests tar with typeflag field with all characters
 * File without data
 * Single file in archive
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_typeflag_field(char* extractor) {
    printf("Testing the field 'typeflag' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int j;
    char typeflag[1];
    // By default, the typeflag is 0
    memset(typeflag, '0', sizeof(typeflag));

    struct tar_t header;

    // Loop through each position in the mode and replace by a character from 0x00 to 0xFF

    for (j = 0x00; j <= 0xFF; j++) {

        typeflag[0] = (char) j;

        // Generate a header with other fields that are correct, only manipulate the mode field
        generate_tar_header(&header, "file.txt", "0000664", "0001750", "0001750", "00000000062",
                            "14413537165", typeflag, "", "ustar", "00", "michal", "michal");

        calculate_checksum(&header);


        write_tar_file("test_typeflag.tar", &header);


        if (extract(extractor, " test_typeflag.tar") == 1 ) {
            // The extractor has crashed
            rename("test_typeflag.tar", "success_typeflag.tar");
            // Delete the extracted file
            remove("file.txt");
            // return 1 to stop the execution as one crash is enough
            return 1;
        } else {
            // Delete the extracted file
            remove("file.txt");
            // Keep going, maybe next character will make it crash
        }
    }


    remove("test_typeflag.tar");
    return 0;
}


/**
 * Tests tar with linkname field with all characters at each position (one position by one, not all combinations)
 * File without data
 * Single file in archive
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_linkname_field(char* extractor) {
    printf("Testing the field 'linkname' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char linkname[99];
    // By default, the linkname is 99 times "a" as the linkname has to be null-terminated
    memset(linkname, 'a', sizeof(linkname));

    struct tar_t header;

    // Loop through each position in the mode and replace by a character from 0x00 to 0xFF
    for (i = 0; i < 99; i++) {
        for (j = 0x00; j <= 0xFF; j++) {

            linkname[i] = (char) j;

            // Generate a header with other fields that are correct, only manipulate the mode field
            generate_tar_header(&header, "file.txt", "0000664", "0001750", "0001750", "00000000062",
                                "14413537165", "8", linkname, "ustar", "00", "michal", "michal");

            calculate_checksum(&header);


            write_tar_file("test_linkname.tar", &header);


            if (extract(extractor, " test_linkname.tar") == 1 ) {
                // The extractor has crashed
                rename("test_linkname.tar", "success_linkname.tar");
                // Delete the extracted file
                remove("file.txt");
                // return 1 to stop the execution as one crash is enough
                return 1;
            } else {
                // Delete the extracted file
                remove("file.txt");
                // Keep going, maybe next character or next position will make it crash
            }
        }
        linkname[i] = 'a';
    }

    remove("test_linkname.tar");
    return 0;
}


/**
 * Tests tar with magic field with all characters at each position (one position by one, not all combinations)
 * File without data
 * Single file in archive
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_magic_field(char* extractor) {
    printf("Testing the field 'magic' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char magic[5];
    // By default, the magic is 5 times "a" as the magic has to be null-terminated
    memset(magic, 'a', sizeof(magic));

    struct tar_t header;

    // Loop through each position in the magic and replace by a character from 0x00 to 0xFF
    for (i = 0; i < 5; i++) {
        for (j = 0x00; j <= 0xFF; j++) {

            magic[i] = (char) j;

            // Generate a header with other fields that are correct, only manipulate the magic field
            generate_tar_header(&header, "file.txt", "0000664", "0001750", "0001750", "00000000062",
                                "14413537165", "8", "", magic, "00", "michal", "michal");

            calculate_checksum(&header);


            write_tar_file("test_magic.tar", &header);


            if (extract(extractor, " test_magic.tar") == 1 ) {
                // The extractor has crashed
                rename("test_magic.tar", "success_magic.tar");
                // Delete the extracted file
                remove("file.txt");
                // return 1 to stop the execution as one crash is enough
                return 1;
            } else {
                // Delete the extracted file
                remove("file.txt");
                // Keep going, maybe next character or next position will make it crash
            }
        }
        magic[i] = 'a';
    }

    remove("test_magic.tar");
    return 0;
}


/**
 * Tests tar with version field with all characters at each position (one position by one, not all combinations)
 * File without data
 * Single file in archive
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_version_field(char* extractor) {
    printf("Testing the field 'version' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char version[2];
    // By default, the version is 2 times "0" 
    memset(version, '0', sizeof(version));

    struct tar_t header;

    // Loop through each position in the version and replace by a character from 0x00 to 0xFF
    for (i = 0; i < 2; i++) {
        for (j = 0x00; j <= 0xFF; j++) {

            version[i] = (char) j;

            // Generate a header with other fields that are correct, only manipulate the version field
            generate_tar_header(&header, "file.txt", "0000664", "0001750", "0001750", "00000000062",
                                "14413537165", "8", "", "ustar", version, "michal", "michal");

            calculate_checksum(&header);


            write_tar_file("test_version.tar", &header);


            if (extract(extractor, " test_version.tar") == 1 ) {
                // The extractor has crashed
                rename("test_version.tar", "success_version.tar");
                // Delete the extracted file
                remove("file.txt");
                // return 1 to stop the execution as one crash is enough
                return 1;
            } else {
                // Delete the extracted file
                remove("file.txt");
                // Keep going, maybe next character or next position will make it crash
            }
        }
        version[i] = '0';
    }

    remove("test_version.tar");
    return 0;
}


/**
 * Tests tar with uname field with all characters at each position (one position by one, not all combinations)
 * File without data
 * Single file in archive
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_uname_field(char* extractor) {
    printf("Testing the field 'uname' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char uname[31];
    // By default, the uname is 31 times "a" as the uname has to be null-terminated
    memset(uname, 'a', sizeof(uname));

    struct tar_t header;

    // Loop through each position in the uname and replace by a character from 0x00 to 0xFF
    for (i = 0; i < 31; i++) {
        for (j = 0x00; j <= 0xFF; j++) {

            uname[i] = (char) j;

            // Generate a header with other fields that are correct, only manipulate the uname field
            generate_tar_header(&header, "file.txt", "0000664", "0001750", "0001750", "00000000062",
                                "14413537165", "8", "", "ustar", "00", uname, "michal");

            calculate_checksum(&header);


            write_tar_file("test_uname.tar", &header);


            if (extract(extractor, " test_uname.tar") == 1 ) {
                // The extractor has crashed
                rename("test_uname.tar", "success_uname.tar");
                // Delete the extracted file
                remove("file.txt");
                // return 1 to stop the execution as one crash is enough
                return 1;
            } else {
                // Delete the extracted file
                remove("file.txt");
                // Keep going, maybe next character or next position will make it crash
            }
        }
        uname[i] = '0';
    }

    remove("test_uname.tar");
    return 0;
}


/**
 * Tests tar with gname field with all characters at each position (one position by one, not all combinations)
 * File without data
 * Single file in archive
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_gname_field(char* extractor) {
    printf("Testing the field 'gname' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char gname[31];
    // By default, the gname is 31 times "a" as the gname has to be null-terminated
    memset(gname, '0', sizeof(gname));

    struct tar_t header;

    // Loop through each position in the gname and replace by a character from 0x00 to 0xFF
    for (i = 0; i < 31; i++) {
        for (j = 0x00; j <= 0xFF; j++) {

            gname[i] = (char) j;

            // Generate a header with other fields that are correct, only manipulate the gname field
            generate_tar_header(&header, "file.txt", "0000664", "0001750", "0001750", "00000000062",
                                "14413537165", "8", "", "ustar", "00", "michal", gname);

            calculate_checksum(&header);


            write_tar_file("test_gname.tar", &header);


            if (extract(extractor, " test_gname.tar") == 1 ) {
                // The extractor has crashed
                rename("test_gname.tar", "success_gname.tar");
                // Delete the extracted file
                remove("file.txt");
                // return 1 to stop the execution as one crash is enough
                return 1;
            } else {
                // Delete the extracted file
                remove("file.txt");
                // Keep going, maybe next character or next position will make it crash
            }
        }
        gname[i] = '0';
    }

    remove("test_gname.tar");
    return 0;
}


/**
 * Tests tar with checksum field with all characters at each position (one position by one, not all combinations)
 * File without data
 * Single file in archive
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_checksum_field(char* extractor) {
    printf("Testing the field 'checksum' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char checksum_str[8];
    // By default, the checksum is 6 times "a" as the checksum has to be terminated by "\0 "
    memset(checksum_str, '0', sizeof(checksum_str));


    struct tar_t header;

    // Loop through each position (but the last two one) in the checksum and replace by a character from 0x00 to 0xFF
    for (i = 0; i < 6; i++) {
        for (j = 0x00; j <= 0xFF; j++) {

            checksum_str[i] = (char) j;

            // Generate a header with other fields that are correct, only manipulate the checksum field
            generate_tar_header(&header, "file.txt", "0000664", "0001750", "0001750", "00000000062",
                                "14413537165", "8", "", "ustar", "00", "michal", "michal");

            // Write the checksum instead of calculating the correct one
            checksum_str[6] = '\0';
            checksum_str[7] = '\x20';

            // memcpy instead of strncpy as it does not care about null terminators
            memcpy(header.chksum, checksum_str, sizeof(header.chksum));

            write_tar_file("test_checksum.tar", &header);


            if (extract(extractor, " test_checksum.tar") == 1 ) {
                // The extractor has crashed
                rename("test_checksum.tar", "success_checksum.tar");
                // Delete the extracted file
                remove("file.txt");
                // return 1 to stop the execution as one crash is enough
                return 1;
            } else {
                // Delete the extracted file
                remove("file.txt");
                // Keep going, maybe next character or next position will make it crash
            }
        }
        checksum_str[i] = '0';
    }

    remove("test_checksum.tar");
    return 0;
}


/**
 * Tests all fields in the header to see if they accept the whole range of characters from 0x00 to 0xFF
 * On files without data
 * One file in archive
 * @param extractor the extractor that will be used
 */
void test_fields_for_all_characters(char* extractor) {
    // 1. Test the file name field
    if (test_filename_field(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! Some non-ascii characters in the file name field caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with the file name field.~~~~~\033[0m\n\n");
    }

    // 2. Test the mode field
    if (test_mode_field(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! Some characters in the mode field caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with the mode field.~~~~~\033[0m\n\n");
    }

    // 3. Test the uid field
    if (test_uid_field(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! Some characters in the uid field caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with the uid field.~~~~~\033[0m\n\n");
    }

    // 4. Test the gid field
    if (test_gid_field(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! Some characters in the gid field caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with the gid field.~~~~~\033[0m\n\n");
    }

    // 5. Test the size field
    if (test_size_field(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! Some characters in the size field caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with the size field.~~~~~\033[0m\n\n");
    }

    // 6. Test the mtime field
    if (test_mtime_field(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! Some characters in the mtime field caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with the mtime field.~~~~~\033[0m\n\n");
    }

    // 7. Test the typeflag field
    if (test_typeflag_field(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! Some characters in the typeflag field caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with the typeflag field.~~~~~\033[0m\n\n");
    }

    // 8. Test the linkname field
    if (test_linkname_field(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! Some characters in the linkname field caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with the linkname field.~~~~~\033[0m\n\n");
    }

    // 9. Test the magic field
    if (test_magic_field(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! Some characters in the magic field caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with the magic field.~~~~~\033[0m\n\n");
    }

    // 10. Test the version field
    if (test_version_field(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! Some characters in the version field caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with the version field.~~~~~\033[0m\n\n");
    }

    // 11. Test the uname field
    if (test_uname_field(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! Some characters in the uname field caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with the uname field.~~~~~\033[0m\n\n");
    }

    // 12. Test the gname field
    if (test_gname_field(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! Some characters in the gname field caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with the gname field.~~~~~\033[0m\n\n");
    }

    // 13. Test the checksum field
    if (test_checksum_field(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! Some characters in the checksum field caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with the checksum field.~~~~~\033[0m\n\n");
    }

}

/**
 * Tests tar with checksum field with incorrect but octal value
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_wrong_checksum_value(char* extractor) {
    printf("Testing a header with a wrong checksum (octal value).\n"
           "        > File without data, as the data is not used to compute the checksum.\n"
           "        > Single file in archive.\n");

    struct tar_t header;

    // Generate a header with other fields that are correct, only manipulate the mode field
    generate_tar_header(&header, "file.txt", "0000664", "0001750", "0001750", "00000000062",
                        "14413537165", "0", "", "ustar", "00", "michal", "michal");

    // Write an incorrect checksum instead of calculating the correct one
    char checksum_str[8] = "741652";
    checksum_str[6] = '\0';
    checksum_str[7] = '\x20';

    // memcpy instead of strncpy as it does not care about null terminators
    memcpy(header.chksum, checksum_str, sizeof(header.chksum));

    write_tar_file("test_wrong_checksum.tar", &header);

    if (extract(extractor, " test_wrong_checksum.tar") == 1 ) {
        // The extractor has crashed
        rename("test_wrong_checksum.tar", "success_wrong_checksum.tar");
        // Delete the extracted file
        remove("file.txt");
        // return 1 to stop the execution as one crash is enough
        return 1;
    } else {
        // Delete the extracted file
        remove("file.txt");
    }

    remove("test_wrong_checksum.tar");

    return 0;
}


/**
 * Tests tar with checksum field with incorrect and non-octal value
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_wrong_checksum_value_non_octal(char* extractor) {
    printf("Testing a header with a wrong checksum (non-octal value).\n"
           "        > File without data, as the data is not used to compute the checksum.\n"
           "        > Single file in archive.\n");

    struct tar_t header;

    // Generate a header with other fields that are correct, only manipulate the mode field
    generate_tar_header(&header, "file.txt", "0000664", "0001750", "0001750", "00000000062",
                        "14413537165", "0", "", "ustar", "00", "michal", "michal");

    // Write an incorrect checksum instead of calculating the correct one
    char checksum_str[8] = "791852";
    checksum_str[6] = '\0';
    checksum_str[7] = '\x20';

    // memcpy instead of strncpy as it does not care about null terminators
    memcpy(header.chksum, checksum_str, sizeof(header.chksum));

    write_tar_file("test_wrong_checksum2.tar", &header);

    if (extract(extractor, " test_wrong_checksum2.tar") == 1 ) {
        // The extractor has crashed
        rename("test_wrong_checksum2.tar", "success_wrong_checksum2.tar");
        // Delete the extracted file
        remove("file.txt");
        // return 1 to stop the execution as one crash is enough
        return 1;
    } else {
        // Delete the extracted file
        remove("file.txt");
    }

    remove("test_wrong_checksum2.tar");

    return 0;
}


/**
 * Tests tar with checksum field with incorrect value and wrongly terminated
 * @param extractor the extractor that will be used
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_wrong_checksum_ending(char* extractor) {
    printf("Testing a header with a wrong checksum (not ended by '0x00 0x20').\n"
           "        > File without data, as the data is not used to compute the checksum.\n"
           "        > Single file in archive.\n");

    struct tar_t header;

    // Generate a header with other fields that are correct, only manipulate the mode field
    generate_tar_header(&header, "file.txt", "0000664", "0001750", "0001750", "00000000062",
                        "14413537165", "0", "", "ustar", "00", "michal", "michal");

    // Write an incorrect checksum instead of calculating the correct one
    char checksum_str[8] = "72125211";

    // memcpy instead of strncpy as it does not care about null terminators
    memcpy(header.chksum, checksum_str, sizeof(header.chksum));

    write_tar_file("test_wrong_checksum3.tar", &header);

    if (extract(extractor, " test_wrong_checksum3.tar") == 1 ) {
        // The extractor has crashed
        rename("test_wrong_checksum3.tar", "success_wrong_checksum3.tar");
        // Delete the extracted file
        remove("file.txt");
        // return 1 to stop the execution as one crash is enough
        return 1;
    } else {
        // Delete the extracted file
        remove("file.txt");
    }

    remove("test_wrong_checksum3.tar");

    return 0;
}


/**
 * Test different possibilities of crashes that could be caused by the checksum field
 * On files without data
 * One file in archive
 */
void test_checksum(char* extractor) {

    // 1. Test if a header with an incorrect (but octal) checksum value will make it crash
    if (test_wrong_checksum_value(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! An incorrect (octal) checksum value has caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with an incorrect (octal) checksum value.~~~~~\033[0m\n\n");

    }

    // 2. Test if a header with an incorrect (but non-octal) checksum value will make it crash
    if (test_wrong_checksum_value_non_octal(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! An incorrect (non-octal) checksum value has caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with an incorrect (non-octal) checksum value.~~~~~\033[0m\n\n");
    }

    // 3. Test if a header with an incorrect (octal) checksum value, not ended with '0x00 0x20' will make it crash
    if (test_wrong_checksum_ending(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! An incorrect (octal) checksum value not ended with '0x00 0x20' has caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with an incorrect (octal) checksum value not ended with '0x00 0x20'.~~~~~\033[0m\n\n");
    }
}


int test_only_null_characters(char* extractor) {
    printf("Testing a header composed of only null characters.\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");

    struct tar_t header;

    // Generate a header with other fields that are correct, only manipulate the mode field
    generate_tar_header(&header, "", "", "", "", "",
                        "", "", "", "ustar", "00", "", "");

    calculate_checksum(&header);

    write_tar_file("test_all_null.tar", &header);

    if (extract(extractor, " test_all_null.tar") == 1 ) {
        // The extractor has crashed
        rename("test_all_null.tar", "success_all_null.tar");
        // Delete the extracted file
        remove("");
        // return 1 to stop the execution as one crash is enough
        return 1;
    } else {
        // Delete the extracted file
        remove("");
    }

    remove("test_all_null.tar");

    return 0;
}


int test_only_null_characters_with_data(char* extractor) {
    printf("Testing a header composed of only null characters. (except magic and version fields)\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");

    struct tar_t header;

    // Generate a header with other fields that are correct, only manipulate the mode field
    generate_tar_header(&header, "", "", "", "", "",
                        "", "", "", "ustar", "00", "", "");

    calculate_checksum(&header);

    write_tar_file_with_data("test_all_null2.tar", &header, "aaaaaaaaaaaaaaaaaaaa", 20);

    if (extract(extractor, " test_all_null2.tar") == 1 ) {
        // The extractor has crashed
        rename("test_all_null2.tar", "success_all_null2.tar");
        // Delete the extracted file
        remove("");
        // return 1 to stop the execution as one crash is enough
        return 1;
    } else {
        // Delete the extracted file
        remove("");
    }

    remove("test_all_null2.tar");

    return 0;
}

void test_null_characters(char* extractor) {

    // 1. Test il all fields in the header with null characters will make it crash (without data)
    if (test_only_null_characters(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! A header full of empty fields has caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with a fully empty header.~~~~~\033[0m\n\n");
    }

    // 2. Test il all fields in the header with null characters will make it crash (without data)
    if (test_only_null_characters_with_data(extractor)) {
        printf("\033[1;32m~~~~~It has crashed ! A header (with data) full of empty fields has caused a crash.~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with a fully empty header (with data).~~~~~\033[0m\n\n");
    }
}


int test_filesize(char* extractor, bool too_big, bool too_small, bool with_data, bool multiple_files) {
    printf("Testing a tar archive for an incorrect filesize value.\n");

    struct tar_t header;

    // Set the size, either too big or too small
    char *size;
    if (too_big) {
        size = "77777779999";
    } else if (too_small) {
        size = "00000000001";
    }

    // Generate a header with other fields that are correct, only manipulate the mode field

    generate_tar_header(&header, "file.txt", "0000664", "0001750", "0001750", size,
                        "14413537165", "0", "", "ustar", "00", "michal", "michal");

    calculate_checksum(&header);

    char* archive_name;

    // File with data
    if (with_data){
        printf(           "        > File with data.\n");
        if (too_big) {
            printf(           "        > Filesize bigger than the actual size.\n");
            if (multiple_files){
                printf(           "        > Multiple files in archive.\n");
                archive_name = "test_size_big3.tar";
            } else {
                printf(           "        > Single file in archive.\n");
                archive_name = "test_size_big1.tar";
            }
        } else if (too_small) {
            printf(           "        > Filesize smaller than the actual size.\n");

            if (multiple_files) {
                printf(           "        > Multiple files in archive.\n");
                archive_name = "test_size_small3.tar";
            } else {
                printf(           "        > Single file in archive.\n");
                archive_name = "test_size_small1.tar";
            }
        }
        write_tar_file_with_data(archive_name, &header, "aaaaaaaaaaaaaaaaaaaa", 20);

    } else {
        // File without data
        printf(           "        > File without data.\n");
        if (too_big) {
            printf(           "        > Filesize bigger than the actual size.\n");
            if (multiple_files){
                printf(           "        > Multiple files in archive.\n");
                archive_name = "test_size_big4.tar";
            } else {
                printf(           "        > Single file in archive.\n");
                archive_name = "test_size_big2.tar";
            }
        } else if (too_small) {
            printf(           "        > Filesize smaller than the actual size.\n");
            if (multiple_files) {
                printf(           "        > Multiple files in archive.\n");
                archive_name = "test_size_small4.tar";
            } else {
                printf(           "        > Single file in archive.\n");
                archive_name = "test_size_small2.tar";
            }
        }
        write_tar_file(archive_name, &header);
    }

    char* space_name = malloc(strlen(archive_name) + 2);
    strcpy(space_name, " ");
    strcat(space_name, archive_name);


    if (extract(extractor, space_name) == 1 ) {
        // The extractor has crashed
        if (strcmp(archive_name, "test_size_big1.tar") == 0) {
            rename("test_size_big1.tar", "success_size_big1.tar");
        } else if (strcmp(archive_name, "test_size_small1.tar") == 0) {
            rename("test_size_small1.tar", "success_size_small1.tar");
        } else if (strcmp(archive_name, "test_size_big2.tar") == 0) {
            rename("test_size_big2.tar", "success_size_big2.tar");
        } else if (strcmp(archive_name, "test_size_small2.tar") == 0) {
            rename("test_size_small2.tar", "success_size_small2.tar");
        } else if (strcmp(archive_name, "test_size_big3.tar") == 0) {
            rename("test_size_big3.tar", "success_size_big3.tar");
        } else if (strcmp(archive_name, "test_size_small3.tar") == 0) {
            rename("test_size_small3.tar", "success_size_small3.tar");
        } else if (strcmp(archive_name, "test_size_big4.tar") == 0) {
            rename("test_size_big4.tar", "success_size_big4.tar");
        } else if (strcmp(archive_name, "test_size_small4.tar") == 0) {
            rename("test_size_small4.tar", "success_size_small4.tar");
        }

        
        // Delete the extracted file(s)
        remove("file.txt");
        if (multiple_files) {
            remove ("file2.txt");
        }
        free(space_name);
        return 1;
    } else {
        // Delete the extracted file(s)
        remove("file.txt");
        if (multiple_files) {
            remove ("file2.txt");
        }
    }

    if (strcmp(archive_name, "test_size_big1.tar") == 0) {
        remove("test_size_big1.tar");
    } else if (strcmp(archive_name, "test_size_big2.tar") == 0) {
        remove("test_size_big2.tar");
    } else if (strcmp(archive_name, "test_size_big3.tar") == 0) {
        remove("test_size_big3.tar");
    } else if (strcmp(archive_name, "test_size_big4.tar") == 0) {
        remove("test_size_big4.tar");
    } else if (strcmp(archive_name, "test_size_small1.tar") == 0) {
        remove("test_size_small1.tar");
    } else if (strcmp(archive_name, "test_size_small2.tar") == 0) {
        remove("test_size_small2.tar");
    } else if (strcmp(archive_name, "test_size_small3.tar") == 0) {
        remove("test_size_small3.tar");
    } else if (strcmp(archive_name, "test_size_small4.tar") == 0) {
        remove("test_size_small4.tar");
    }

    free(space_name);
    return 0;
}


void test_wrong_filesize(char* extractor) {

    // 1. Test filesize value too big, without data, single file in archive
    if (test_filesize(extractor, true, false, false, false)) {
        printf("\033[1;32m~~~~~It has crashed ! A too big filesize value caused a crash (no data, single file).~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with filesize value.~~~~~\033[0m\n\n");
    }

    // 2. Test filesize value too small, without data, single file in archive
    if (test_filesize(extractor, false, true, false, false)) {
        printf("\033[1;32m~~~~~It has crashed ! A too small filesize value caused a crash (no data, single file).~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with filesize value.~~~~~\033[0m\n\n");
    }

    // 3. Test filesize value too big, with data, single file in archive
    if (test_filesize(extractor, true, false, true, false)) {
        printf("\033[1;32m~~~~~It has crashed ! A too big filesize value caused a crash (with data, single file).~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with filesize value.~~~~~\033[0m\n\n");
    }

    // 4. Test filesize value too big, with data, single file in archive
    if (test_filesize(extractor, false, true, true, false)) {
        printf("\033[1;32m~~~~~It has crashed ! A too small filesize value caused a crash (with data, single file).~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with filesize value.~~~~~\033[0m\n\n");
    }

    // 5. Test filesize value too big, without data, multiple files in archive
    if (test_filesize(extractor, true, false, false, true)) {
        printf("\033[1;32m~~~~~It has crashed ! A too big filesize value caused a crash (no data, multiple files).~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with filesize value.~~~~~\033[0m\n\n");
    }

    // 6. Test filesize value too small, without data, multiple files in archive
    if (test_filesize(extractor, false, true, false, true)) {
        printf("\033[1;32m~~~~~It has crashed ! A too small filesize value caused a crash (no data, multiple files).~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with filesize value.~~~~~\033[0m\n\n");
    }

    // 7. Test filesize value too big, with data, multiple files in archive
    if (test_filesize(extractor, true, false, true, true)) {
        printf("\033[1;32m~~~~~It has crashed ! A too big filesize value caused a crash (with data, multiple files).~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with filesize value.~~~~~\033[0m\n\n");
    }

    // 8. Test filesize value too big, with data, multiple files in archive
    if (test_filesize(extractor, false, true, true, true)) {
        printf("\033[1;32m~~~~~It has crashed ! A too small filesize value caused a crash (with data, multiple files).~~~~~\033[0m\n\n");
    } else {
        printf("\033[1;31m~~~~~No issues found with filesize value.~~~~~\033[0m\n\n");
    }
}


int main(__attribute__((unused)) int argc, char* argv[]) {

    // Test all fields in the header to see if they accept the whole range of characters from 0x00 to 0xFF (one file, no data)
    //test_fields_for_all_characters(argv[1]);

    // Test different possibilities of crashes that could be caused by the checksum field
    test_checksum(argv[1]);

    // Test all fields if they can work when composed of only null characters
    test_null_characters(argv[1]);

    // Test conducted on filesize (with and without data)
    test_wrong_filesize(argv[1]);

    // TODO : test if filesize can not match with the actual file size (more data than file size)

    // TODO : test if filesize can not match with the actual file size (more data than file size) - with two files in one archive (and data)

    // TODO : test if data can be non-padded

    // TODO : check if a header + non-padded data + header + data will work

    // TODO : check if all numerical values can be negative (size, mtime, mode, uid, gid, checksum)

    // TODO : test all fields if they can end without the null character

    // TODO : try extracting a file that does not exist

    return 0;
}