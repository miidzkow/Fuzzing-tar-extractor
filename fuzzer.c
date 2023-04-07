#include <stdio.h>
#include <string.h>
#include <stdint-gcc.h>


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
unsigned int calculate_checksum(struct tar_t* entry){
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
                         const char *uname, const char *gname)
{
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

int extract(char* extractor)
{
    int rv = 0;
    char cmd[51];
    strncpy(cmd, extractor, 25);
    cmd[26] = '\0';
    strncat(cmd, " testarchive.tar", 25);
    char buf[33];
    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return -1;
    }

    if(fgets(buf, 33, fp) == NULL) {
        printf("No output\n");
        goto finally;
    }
    if(strncmp(buf, "*** The program has crashed ***\n", 33)) {
        printf("Not the crash message\n");
        goto finally;
    } else {
        printf("Crash message\n");
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

int main(int argc, char* argv[]){

    struct tar_t header;

    // Generate header
    generate_tar_header(&header, "file.txt", "0000664", "0001750", "0001750", "00000000062",
                        "14413537165", "0", "", "ustar", "00", "michal", "michal");

    // Compute the checksum
    calculate_checksum(&header);

    // Write tar archive to file
    write_tar_file("testarchive.tar", &header);


    extract(argv[1]);

    return 0;
};