#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include <math.h>
#include <arpa/inet.h>
#include "x.h"

struct header {
   char name[100];
   char mode[8];
   char uid[8];
   char gid[8];
   char size[12];
   char mtime[12];
   char chksum[8];
   char typeflag;
   char linkname[100];
   char magic[6];
   char version[2];
   char uname[32];
   char gname[32];
   char devmajor[8];
   char devminor[8];
   char prefix[155];
   char padding[12];
};


int main(int argc, char *argv[]){
   char ** f_list;
   int t_files;
   int i, flags;
   int v = 1; 
   flags = strlen(argv[1]);
   int status = 0;
   if (argc < 2){
      printf("%s [ctxvS]f tarfile [ path [...]]\n", argv[0]);
      exit(1);
   }
   else{
      for(i = 0; i < flags; i++){
         if(1){
            status = 1;
         }
         else{
            printf("Usage error");
            exit(1);
         }
      }
   }

   if (status == 1){
      t_files = (argc - 3);
      f_list = (argv + 3);
      extract(argv[2], t_files, f_list, v);
   }
}




int opener(char *file, char permission){
      int fp;
      if((fp = open(file, permission)) < 0){
         perror("opener error");
         exit(1);
      }
      return fp;
}

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
   //printf("Path %s\n", path);
   //printf("Permissions %d\n", permissions);
   if ((fp = open(path, O_WRONLY | O_CREAT | O_TRUNC, permissions)) < 0) {
      perror("permission open error");
      exit(1);
   }

   return fp;
}




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
//char * pathget(char *name, int v, char prefix[], char name1[]){
char * pathget(char *name, int v, struct header h){
   char path [256];
   int x;
   for(x = 0; x <= 256; ++x){
      path[x] = '\0';
   }
   //for(i = 0; i < 15; i++ )
   //      printf("Path Prefix: %c | Name: %c |\n", h.prefix[i], h.name[i]);
   strcpy(name, path);
   strncat(name, h.prefix, 155);

   if(strlen(name) > 0){
      strncat(name, "/", 1);
   }
   strncat(name, h.name, 100);
   if (v == 1){
      printf("%s\n", name);
   }


   return name;
}

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


char * t_perm(char * perm_point, struct header h, char mode []){
   int i, x, length;

   typeflag = (h.typeflag);
   switch(typeflag){

      case '2':
         *(perm_point + 0) = 'l';
         break;

      case '5':
         *(perm_point + 0) = 'd';
         break;
   }

   length = strlen(h.mode)
   //copies the permissions into an array
   for(i = (length - 3), x = 1; i >= (length-1); i++, x + 3){

      if((mode[i] == '4') || (mode[i] ==  '5')  || (mode[i] == '6') || (mode[i] ==  '7'))
         *(perm_point + x) = "r";
      else
         *(perm_point + x) = "_";

      if((mode[i] == '2') || (mode[i] ==  '3')  || (mode[i] == '6') || (mode[i] ==  '7'))
         *(perm_point + (x + 1)) = "w";
      else
         *(perm_point + (x + 1)) = "_";

      if((mode[i] == '1') || (mode[i] ==  '3')  || (mode[i] == '5') || (mode[i] ==  '7'))
         *(perm_point + (x + 2)) = "x";
      else
         *(perm_point + (x + 2)) = "_";
   }
   *(perm_point + 10) = "\0";
   return perm_point;
}



void t_names(char *file, int t_files, char **f_list, int v){
   char permission_array[11];
   char * perm_point;
   struct header h;
   time_t time;
   struct tm time_str;
   int month, year, size;
   char group[18];
   char epath [256];
   char * epathpoint;
   int file_sz;

   fp = opener(file, O_RDONLY);
    while(reader(fp, &h, 512)){
         epathpoint = epath;
         epathpoint = pathget(epathpoint, v, h);
         if(strlen(epathpoint) == 0)
            break;
         if ((t_files == 0 || has(epathpoint, t_files, f_list))){

            if(v){
               //get permission numbers
               perm_point = t_perm(perm_point, h, h.mode);

               size = strtol((contents.size), NULL, 8);

               //time
               time = ((time_t)strtol(contents.mtime, NULL, 8));
               localtime_r(&time, &time_str);
               year = time_str.tm_year + 1900;
               month = time_str.tm_mon + 1;

               strcat(group, h.uname);
               strcat(group, "/\0");
               strcat(group, h.gname);
               printf("%10s %-17s %8d %04d-%02d-%02d %02d:%02d %s\n",
                                             permissions, usergroup,
                                             size, year, month, time_str.tm_mday,
                                             time_str.tm_hour, time_str.tm_min,
                                             epathpoint);


            }
            else{
               printf("%s\n", name);
            }
         }
         if (!(t_files == 0 || has(epathpoint, t_files, f_list))) {
            file_sz = strtol(h.size, NULL, 8);
            lseek(fp, 512 *(file_sz/512 + ((file_sz%512) ?1:0)), SEEK_CUR);
         }
      }
}





void extract(char *file, int t_files, char **f_list, int v){

   int i, fp,  fpout;
   struct header h;
   int file_sz, file_sz2, file_block;
   char epath [256];
   char * epathpoint;
   char typeflag;
   char buff2[512];

   fp = opener(file, O_RDONLY);
   while(reader(fp, &h, 512)){
         epathpoint = epath;
         epathpoint = pathget(epathpoint, v, h);
         //printf("Pathname Pointer: %s\n", epathpoint);
         if(strlen(epathpoint) == 0)
            break;
         if ((t_files == 0 || has(epathpoint, t_files, f_list))){
            typeflag = (h.typeflag);
            switch(typeflag){

               case '2':
                  if(symlink(h.linkname, epathpoint) < 0){
                     perror("symlink error");
                     exit(1);
                  }
                  break;

               case '5':
                  mkdir(epathpoint, S_IRWXU | S_IRWXG | S_IRWXO);
                  break;

               default:                                     //default for normal files
                  fpout = fp_permissions(epathpoint, h.mode);

                  file_sz = strtol(h.size, NULL, 8);
                  file_sz2 = file_sz;
                  file_block = (file_sz) / 512 + ((file_sz %512) ? 1 : 0);
                  for(i = 0; i < file_block ; i++){
                     if(file_sz2 < 512){
                        reader(fp, &buff2, 512);
                        writer(fpout, &buff2, file_sz2);
                     }

                     else{
                        reader(fp, &buff2, 512);
                        writer(fpout, &buff2, 512);
                        file_sz2 -= 512;
                     }
                  }
               close(fpout);
               }
         }

         if (!(t_files == 0 || has(epathpoint, t_files, f_list))) {
            file_sz = strtol(h.size, NULL, 8);
            lseek(fp, 512 *(file_sz/512 + ((file_sz%512) ?1:0)), SEEK_CUR);
         }
   }
}

                                       

