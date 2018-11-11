
#include "mytar.h"

int create_tar(int fd, char *basepath, int verb, mytar_t **archive) {
    DIR *d;
    struct dirent *dir;
    struct stat buf;
    char path[PATH_LIMIT];
    char clean_path[PATH_LIMIT];
    struct passwd *pws;
    struct group *grp;
    int jump = 148;
    int rfd = 0;
    int buf_size = 0;
    char boof[BLOCKSIZE] = { 0 };

    /* Base case */
    if (!(d = opendir(basepath))) {
        return 1;
    }
    /* Read through each file in directory */
    while ((dir = readdir(d)) != NULL) {
        /* Recurse through but not for "." and ".." */
        if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
            /* Add paths to pathname */
            strcpy(path, basepath);
            strcat(path, "/");
            strcat(path, dir->d_name);

            if ((lstat(path, &buf)) == -1) {
                perror("Error: ");
            }

            *archive = fill_metadata(*archive, buf, path);
            *archive = fill_block(*archive);

            /* Set condition value */
            *archive = get_checksum(*archive);
            memcpy((*archive)->block + jump, (*archive)->chksum, COMMON_SIZE);

            /* Write the header portion */
            write(fd, (*archive)->block, BLOCKSIZE);
    
            /* Write the contents of the file if there is content */
            if ((rfd = open(path, O_RDONLY)) == -1) {
                perror("Error: ");
                return 1;
            }    
    
            while ((buf_size = read(rfd, boof, BLOCKSIZE)) > 0) {
                write(fd, boof, BLOCKSIZE);
                memset(boof, 0, BLOCKSIZE);
            }

            close(rfd);

            /* Check for symlinks */
            if (!S_ISLNK(buf.st_mode)) { 
                create_tar(fd, path, verb, archive);
            }
        }
    }
    closedir(d);
    return 0;
}
int first_dir(int fd, char *path, int verb, mytar_t **archive) {
    struct stat buf;
    int i = 0;
    int jump = 148;
    int rfd = 0;
    int buf_size = 0;
    char boof[BLOCKSIZE] = { 0 };
    /* Get info on first directory */
    if (lstat(path, &buf) == -1) {
        fprintf(stderr, "Unable to stat: %s\n", path);
        return 1;
    }
    /* Fill the data of the archive */
    *archive = fill_metadata(*archive, buf, path);
    strcat((*archive)->name, "/");
    *archive = fill_block(*archive);

    /* Set condition value */
    *archive = get_checksum(*archive);
    memcpy((*archive)->block + jump, (*archive)->chksum, COMMON_SIZE);

    /* Write the header portion */
    write(fd, (*archive)->block, BLOCKSIZE);
    
    /* Write the contents of the file if there is content */
    if ((rfd = open(path, O_RDONLY)) == -1) {
        perror("Error: ");
        return 1;
    }    
    
    while ((buf_size = read(rfd, boof, BLOCKSIZE)) > 0) {
        write(fd, boof, BLOCKSIZE);
        memset(boof, 0, BLOCKSIZE);
    }
    
    close(rfd);
    return 0;
}

int create_regfile(int fd, char *path, int verb, mytar_t **archive) {
    struct stat buf;
    int i = 0;
    int jump = 148;
    int rfd = 0;
    int buf_size = 0;
    char boof[BLOCKSIZE] = { 0 };

    if ((lstat(path, &buf)) == -1) {
        perror("Error: ");
        return 1;
    }
    /* Clear everything in the archive */
    *archive = fill_metadata(*archive, buf, path);
    *archive = fill_block(*archive);
    /* Set condition value */
    *archive = get_checksum(*archive);
    memcpy((*archive)->block + jump, (*archive)->chksum, COMMON_SIZE);

    /* Write the header portion */
    write(fd, (*archive)->block, BLOCKSIZE);
    
    /* Write the contents of the file if there is content */
    if ((rfd = open(path, O_RDONLY)) == -1) {
        perror("Error: ");
        return 1;
    }    
    
    while ((buf_size = read(rfd, boof, BLOCKSIZE)) > 0) {
        write(fd, boof, BLOCKSIZE);
        memset(boof, 0, BLOCKSIZE);
    }
    
    close(rfd);
    
    return 0;
}

mytar_t *fill_metadata(mytar_t *archive, struct stat buf, char *path) {
    struct passwd *pws;
    struct group *grp; 
    int i = 0;
    int path_size = strlen(path);
    int tot_size = 100; /* Size of the name char array */

    /* Path fits perfectly into name field */
    if (path_size == tot_size) {
        /* Clear path name */
        memset(archive->name, 0, FILENAME_SIZE);
        memcpy(archive->name, path, FILENAME_SIZE);
    }
    /* Path fits inside name field */
    else if (path_size < tot_size) {
        /* Clear path name */
        memset(archive->name, 0, FILENAME_SIZE);
        memcpy(archive->name, path, path_size);
        memset(archive->name + FILENAME_SIZE - 1,'\0', 1); 
    }
    /* Path overflows into prefix */
    
    else {  
        memset(archive->name, 0, FILENAME_SIZE);
        memset(archive->prefix, 0 , PREFIX_SIZE);
        strncpy(archive->name, path, tot_size);
        strcpy(archive->prefix, path + tot_size);
        if (path_size < PATH_LIMIT) {
            strcat(archive->prefix, "\0");
        }
    }

    sprintf((archive)->mode, "%07o", buf.st_mode & MODE_MASK);
    sprintf((archive)->uid, "%07o", buf.st_uid);           
    sprintf((archive)->gid, "%07o", buf.st_gid);           
    sprintf((archive)->size, "%011o", (int) buf.st_size);
    sprintf((archive)->mtime, "%011o", (int) buf.st_mtime);
    strcpy((archive)->magic, "ustar\0");
    strcpy((archive)->version, "00");

    /* User name and group name */
    pws = getpwuid(buf.st_uid);
    strcpy((archive)->uname, pws->pw_name);
    grp = getgrgid(buf.st_gid);

    strcpy((archive)->gname, grp->gr_name);

    /* Set devmajor and devminor to null */
    memset((archive)->devmajor, 0, COMMON_SIZE);
    memset((archive)->devminor, 0, COMMON_SIZE);
    /*
    sprintf((archive)->devmajor, "%08o", '\0');
    sprintf((archive)->devminor, "%08o", '\0');
    */
    
            

    /* Symlink */
    if (S_ISLNK(buf.st_mode)) {
        sprintf((archive)->size, "%011o", 0);
        (archive)->typeflag[0] = SYMLINK;
        if (readlink(path, archive->linkname, tot_size) < 0) {
            perror("Error: ");
        }
    }
    /* Directory */
    else if (S_ISDIR(buf.st_mode)) {
        sprintf((archive)->size, "%011o", 0);
        (archive)->typeflag[0] = DIRECTORY;
    }
 
    /* File */
    else {
        (archive)->typeflag[0] = REGFILE;
    }
    return archive;
}

mytar_t *fill_block(mytar_t *archive) {
    /* Perhaps the most inefficient filling a header block */
    int i = 0;
    int header_block = 500;
    int jump = 0;
    char chksum_buf[COMMON_SIZE];
    char null_buf[SIZE_SIZE] = { 0 };
    
    /* Set checksum to all spaces */
    for (i = 0; i < COMMON_SIZE; i++) {
        chksum_buf[i] = ' ';
    }
    
    /* Memcpy information into the block attribute */
    memcpy(archive->block, archive->name, FILENAME_SIZE);
    jump += FILENAME_SIZE;
    memcpy(archive->block + jump, archive->mode, COMMON_SIZE);
    jump += COMMON_SIZE;
    memcpy(archive->block + jump, archive->uid, COMMON_SIZE);
    jump += COMMON_SIZE;
    memcpy(archive->block + jump, archive->gid, COMMON_SIZE);
    jump += COMMON_SIZE;
    memcpy(archive->block + jump, archive->size, SIZE_SIZE);
    jump += SIZE_SIZE;
    memcpy(archive->block + jump, archive->mtime, SIZE_SIZE);
    jump += SIZE_SIZE;
    memcpy(archive->block + jump, chksum_buf, COMMON_SIZE);
    jump += COMMON_SIZE;
    memcpy(archive->block + jump, archive->typeflag, TYPEFLAG_SIZE);
    jump += TYPEFLAG_SIZE;
    memcpy(archive->block + jump, archive->linkname, FILENAME_SIZE);
    jump += FILENAME_SIZE;
    memcpy(archive->block + jump, archive->magic, MAGIC_SIZE);
    jump += MAGIC_SIZE;
    memcpy(archive->block + jump, archive->version, VERSION_SIZE);
    jump += VERSION_SIZE;
    memcpy(archive->block + jump, archive->uname, GROUPNAME_SIZE);
    jump += GROUPNAME_SIZE;
    memcpy(archive->block + jump, archive->gname, GROUPNAME_SIZE);
    jump += GROUPNAME_SIZE;
    memcpy(archive->block + jump, archive->devmajor, COMMON_SIZE);
    jump += COMMON_SIZE;
    memcpy(archive->block + jump, archive->devminor, COMMON_SIZE);
    jump += COMMON_SIZE;
    memcpy(archive->block + jump, archive->prefix, PREFIX_SIZE);
    jump += PREFIX_SIZE;
    memcpy(archive->block + jump, null_buf, SIZE_SIZE);


    return archive;
}

/* Not sure if this function is necessary */
mytar_t *zeroout(mytar_t *archive) {
    char zeroed[200] = { 0 };
    int size_100 = 100;
    int size_8 = 8;
    int size_32 = 32;
    int size_6 = 6;
    int size_12 = 12;
    int size_155 = 155;

    strncpy(archive->name, zeroed, size_100);
    strncpy(archive->mode, zeroed, size_8);
    strncpy(archive->uid, zeroed, size_8);
    strncpy(archive->gid, zeroed, size_8);
    strncpy(archive->size, zeroed, size_12);
    strncpy(archive->mtime, zeroed, size_12);
    strncpy(archive->chksum, zeroed, size_8);
    strncpy(archive->linkname, zeroed, size_100);
    strncpy(archive->uname, zeroed, size_32);
    strncpy(archive->gname, zeroed, size_32);
    strncpy(archive->devmajor, zeroed, size_8);
    strncpy(archive->devminor, zeroed, size_8);
    strncpy(archive->prefix, zeroed, size_155);
    return archive;
}

mytar_t *get_checksum(mytar_t *archive) {
    int i = 0;
    unsigned int check = 0;
    for (i = 0; i < 500; i++) {
        check += (unsigned char) archive->block[i];
    }
    sprintf(archive->chksum, "%.7o", check);
    return archive;
}

mytar_t *make_header() {
    mytar_t *archive = NULL;
    archive = calloc(1, sizeof(mytar_t));
    if (archive == NULL) {
        perror("Error: ");
    }
    return archive;
}


void print_metadata(mytar_t *archive) {
    printf("Name: %s\n", archive->name);
    printf("Mode: %s\n", archive->mode);
    printf("Uid: %s\n", archive->uid);
    printf("Gid: %s\n", archive->gid);
    printf("Size: %s\n", archive->size);
    printf("Mtime: %s\n", archive->mtime);
   /* printf("Chksum: %s\n", archive->chksum);*/
    printf("Typeflag: %s\n", archive->typeflag);
    printf("Linkname: %s\n", archive->linkname);
    printf("Magic: %s\n", archive->magic);
    printf("Version: %s\n", archive->version);
    printf("Username: %s\n", archive->uname);
    printf("Group Name: %s\n", archive->gname);
    printf("Devmajor: %s\n", archive->devmajor);
    printf("Devminor: %s\n", archive->devminor);
    printf("Prefix: %s\n", archive->prefix);
    printf("\n");
}


/* function to open a file*/
int opener(char *file, char permission){
      int fp;
      if((fp = open(file, permission)) < 0){
         perror("opener error");
         exit(1);
      }
      return fp;
}
/* function to set permissions for - extract */
int fp_permissions(char *path, char mode[]){

   int length;
   int permissions = 0;
   int i;
   int fp = 0;
   int ex = 0;
   length = strlen(mode);


   for(i = (length - 1); i >= (length-3); i--){
      if((mode[i] == '1') || (mode[i] ==  '3')  || (mode[i] == '5') || (mode[i] ==  '7')){
         ex = 1;
      }
   }

   if(ex){
      permissions |= (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
   }
   else{
      permissions |= (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
   }

   if ((fp = open(path, O_WRONLY | O_CREAT | O_TRUNC, permissions)) < 0) {
      perror("permission open error");
      exit(1);
   }
   return fp;
}
/* function to check if the file path is in */
int has(char *path, int t_files, char **f_list)
{
   char *point;
   char pather[256];
   char actual[256];
   int  actuallen, x;
   for (x = 0; x < t_files; ++x)
   {
      strncpy(pather, path, 255);
      strncpy(actual, f_list[x], 255);
      actuallen = strlen(actual);

      if (actual[actuallen - 1] == '/')
         actual[actuallen - 1] = '\0';

      if(!strcmp(actual, pather))
         return 1;

      if((point = (rindex(pather, '/')))){
         *point = '\0';
      }

      while(point){
         if((strcmp(pather, actual) == 0))
            return 1;

         if((point = rindex(pather, '/')))
            *point = '\0';

      }
   }

   return 0;
}
/* function to get the path */
char * pathget(char *name, int v, struct header h){
   int x;
   for(x = 0; x <= PATH_LIMIT; ++x){
      *(name + x) = '\0';
   }

   strncat(name, h.prefix, PREFIX_SIZE);

   if(strlen(h.prefix) > 0){
      strncat(name, "/", 1);
   }
   strncat(name, h.name, FILENAME_SIZE);

   return name;
}

/* function to read in */
int reader(int fd, void *buffer, size_t size)
{
   int sz = 0;
   if ((sz = read(fd, buffer, size)) < 0)
   {
      perror("reader error");
      exit(1);
   }
   return sz;
}
/* function to write */
int writer(int fdo, void *buffer, size_t size)
{
   int sz = 0;
   if ((sz = write(fdo, buffer, size)) < 0)
   {
      perror("writer error");
      exit(1);
   }
   return sz;
}

/* functions to check the permissions for - t */
char * t_perm(char * perm_point, struct header h, char mode []){
   int i, x, length;
   char typeflag[1];
   length = strlen(h.mode);
   typeflag[0] = (h.typeflag[0]);
   switch(typeflag[0]){

      case '2':
         *(perm_point + 0) = '1';
         break;

      case '5':
         *(perm_point + 0) = 'd';
         break;

      default:
         *(perm_point + 0) = '-';
         break;
   }
   x = 1;
   /*copies the permissions into an array */
   for(i = (length - 3); i <= (length-1); i++){
      if((h.mode[i] == '4') || (h.mode[i] ==  '5')  || (h.mode[i] == '6') || (h.mode[i] ==  '7'))

         *(perm_point + x) = 'r';
      else
         *(perm_point + x) = '-';

      if((h.mode[i] == '2') || (h.mode[i] ==  '3')  || (h.mode[i] == '6') || (h.mode[i] ==  '7'))
         *(perm_point + (x + 1)) = 'w';
      else
         *(perm_point + (x + 1)) = '-';

      if((h.mode[i] == '1') || (h.mode[i] ==  '3')  || (h.mode[i] == '5') || (h.mode[i] ==  '7'))
         *(perm_point + (x + 2)) = 'x';
      else
         *(perm_point + (x + 2)) = '-';
      x = x + 3;
   }
   //printf(" permission number: %s\n", perm_point);
   *(perm_point + 10) = '\0';
   return perm_point;
}

/* function to set all t list */
void t_names(char *file, int t_files, char **f_list, int v){
   char permission_array[11];
   char * perm_point;
   struct header h;
   time_t time;
   struct tm time_str;
   int month, year, fp, size = 0;
   char group[18];
   char epath [PATH_LIMIT];
   char * epathpoint;
   int file_sz, i;

   fp = opener(file, O_RDONLY);
   while(reader(fp, &h, 512)){
         epathpoint = epath;
         epathpoint = pathget(epathpoint, v, h);
         if(strlen(epathpoint) == 0)
            break;

         if ((t_files == 0 || has(epathpoint, t_files, f_list))){

            if(v){
               /*get permission numbers*/
               perm_point = permission_array;
               perm_point = t_perm(perm_point, h, h.mode);

               size = strtol((h.size), NULL, 8);

               /* time */
               time = ((time_t)strtol(h.mtime, NULL, 8));
               localtime_r(&time, &time_str);
               year = time_str.tm_year + 1900;
               month = time_str.tm_mon + 1;
                
               for(i = 0; i < 18; i++)
                  group[i] = '\0';
               strcat(group, h.uname);
               strcat(group, "/\0");
               strcat(group, h.gname);
               strcat(group, "\0");

               printf("%10s %-17s %8d %04d-%02d-%02d %02d:%02d %s\n",
                                             perm_point, group,
                                             size, year, month, time_str.tm_mday,
                                             time_str.tm_hour, time_str.tm_min,
                                                                                                                                                                            546,4         85%
            }
            else{
               printf("%s\n", epathpoint);
            }
         }
            file_sz = 0;
            file_sz = strtol(h.size, NULL, 8);
            lseek(fp, BLOCKSIZE *(file_sz/BLOCKSIZE + ((file_sz%BLOCKSIZE) ?1:0)), SEEK_CUR);
      }
}


/* function to extract */
void extract(char *file, int t_files, char **f_list, int v){

   int i, fp,  fpout;
   struct header h;
   int file_sz, file_sz2, file_block;
   char epath [PATH_LIMIT];
   char * epathpoint;
   char typeflag[1];
   char buff2[BLOCKSIZE];

   fp = opener(file, O_RDONLY);
      while(reader(fp, &h, BLOCKSIZE)){
         epathpoint = epath;
         epathpoint = pathget(epathpoint, v, h);
         if (v == 1){
            printf("%s\n", epathpoint);
         }

         if(strlen(epathpoint) == 0)
            break;
         if ((t_files == 0 || has(epathpoint, t_files, f_list))){
            typeflag[0] = (h.typeflag[0]);
            printf("typeflag: %d", typeflag[0]);
            switch(typeflag[0]){

               case '2':
                  if(symlink(h.linkname, epathpoint) < 0){
                     perror("symlink error");
                     exit(1);
                  }
                  break;

               case '5':
                  mkdir(epathpoint, S_IRWXU | S_IRWXG | S_IRWXO);
                  break;

               default:
                  fpout = fp_permissions(epathpoint, h.mode);

                  file_sz = strtol(h.size, NULL, 8);
                  file_sz2 = file_sz;
                  file_block = (file_sz) / BLOCKSIZE + ((file_sz %BLOCKSIZE) ? 1 : 0);
                  for(i = 0; i < file_block ; i++){
                     if(file_sz2 < BLOCKSIZE){
                        reader(fp, &buff2, BLOCKSIZE);
                        writer(fpout, &buff2, file_sz2);
                     }

                     else{
                        reader(fp, &buff2, BLOCKSIZE);
                        writer(fpout, &buff2, BLOCKSIZE);
                        file_sz2 -= BLOCKSIZE;
                     }
                  }
               close(fpout);
             }
         }

            file_sz = 0;
            file_sz = strtol(h.size, NULL, 8);
            lseek(fp, BLOCKSIZE *(file_sz/BLOCKSIZE + ((file_sz%BLOCKSIZE) ?1:0)), SEEK_CUR);
         }
   }
}


