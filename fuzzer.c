#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


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

void write_tar_file(const char* filename, struct tar_t* header) {
    // Open file for writing in binary mode
    FILE* file = fopen(filename, "wb");

    // Write the header to the file
    fwrite(header, sizeof(struct tar_t), 1, file);

    // Close the file
    fclose(file);
}

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
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_uid_field(char* extractor) {
    printf("Testing the field 'uid' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char uid[7];
    // By default, the mode is 7 times "0" as the mode has to be null-terminated
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
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_gid_field(char* extractor) {
    printf("Testing the field 'gid' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char gid[7];
    // By default, the mode is 7 times "0" as the mode has to be null-terminated
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
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_size_field(char* extractor) {
    printf("Testing the field 'size' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int i, j;
    char size[11];
    // By default, the mode is 7 times "0" as the mode has to be null-terminated
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
    // By default, the mode is 7 times "0" as the mode has to be null-terminated
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
 * @return 0 if no crash was found, 1 if it crashed
 */
int test_typeflag_field(char* extractor) {
    printf("Testing the field 'typeflag' with all characters for each possible position (position by position).\n"
           "        > File without data.\n"
           "        > Single file in archive.\n");


    int j;
    char typeflag[1];
    // By default, the mode is 7 times "0" as the mode has to be null-terminated
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


int main(__attribute__((unused)) int argc, char* argv[]) {

    // 1. Test the file name field
    if (test_filename_field(argv[1]) == 1) {
        printf("~~~~~It has crashed ! Some non-ascii characters in the file name field caused a crash.~~~~~\n\n");
    } else {
        printf("~~~~~No issues found with the file name field.~~~~~\n\n");
    }

    // 2. Test the mode field
    if (test_mode_field(argv[1]) == 1) {
        printf("~~~~~It has crashed ! Some characters in the mode field caused a crash.~~~~~\n\n");
    } else {
        printf("~~~~~No issues found with the mode field.~~~~~\n\n");
    }

    // 3. Test the uid field
    if (test_uid_field(argv[1]) == 1) {
        printf("~~~~~It has crashed ! Some characters in the uid field caused a crash.~~~~~\n\n");
    } else {
        printf("~~~~~No issues found with the uid field.~~~~~\n\n");
    }

    // 4. Test the gid field
    if (test_gid_field(argv[1]) == 1) {
        printf("~~~~~It has crashed ! Some characters in the gid field caused a crash.~~~~~\n\n");
    } else {
        printf("~~~~~No issues found with the gid field.~~~~~\n\n");
    }

    // 5. Test the size field
    if (test_size_field(argv[1]) == 1) {
        printf("~~~~~It has crashed ! Some characters in the size field caused a crash.~~~~~\n\n");
    } else {
        printf("~~~~~No issues found with the size field.~~~~~\n\n");
    }

    // 6. Test the mtime field
    if (test_mtime_field(argv[1]) == 1) {
        printf("~~~~~It has crashed ! Some characters in the mtime field caused a crash.~~~~~\n\n");
    } else {
        printf("~~~~~No issues found with the mtime field.~~~~~\n\n");
    }

    // 7. Test the typeflag field
    if (test_typeflag_field(argv[1]) == 1) {
        printf("~~~~~It has crashed ! Some characters in the typeflag field caused a crash.~~~~~\n\n");
    } else {
        printf("~~~~~No issues found with the typeflag field.~~~~~\n\n");
    }

    return 0;
}