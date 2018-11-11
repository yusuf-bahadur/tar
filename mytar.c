#include "mytar.h"

int main(int argc, char *argv[]) {
    int i = 0;
    int ending_block = 2;
    int fd = 0;
    int path_arg = 3;

    /* Options supported */
    char c = 0,
        t = 0,
        x = 0,
        v = 0,
        S = 0;

    struct stat buf; /* Use lstat to work with symbollic links */
    char *options = strdup(argv[1]); /* Character options */
    char *tarfile = strdup(argv[2]); /* Tarfile name */
    char null_block[BLOCKSIZE] = { 0 };
    mytar_t *archive = NULL;
    archive = calloc(1, sizeof(mytar_t));


    /* Check the input [ctxvS] */
    for (i = 0; i < strlen(options); i++) {
        if (options[i] == 'c') {
            c = 1;
        }
        else if (options[i] == 't') {
            t = 1;
        }
        else if (options[i] == 'x') {
            x = 1;
        }
        else if (options[i] == 'v') {
            v = 1;
        }
        else if (options[i] == 'S') {
            S = 1;
        }
    }
    /* Create option specified */
    if (c == 1) {
        if ((fd = open(tarfile, O_WRONLY | O_CREAT | O_TRUNC,
        S_IRUSR | S_IWUSR)) == -1) {
            perror("Error: ");
            return 1;
        }
        /* Add the paths to the archive */
        for (i = path_arg; i < argc; i++) {
            if (lstat(argv[i], &buf) < 0) {
                perror("Error: ");
            }
            if (S_ISDIR(buf.st_mode)) {
                first_dir(fd, argv[i], v, &archive);
                create_tar(fd, argv[i], v, &archive);
            }
            else if (S_ISREG(buf.st_mode)) {
                create_regfile(fd, argv[i], v, &archive);
            }
        }
        /* Write the ending block */
        for (i = 0; i < ending_block; i++) {
            if ((write(fd, null_block, BLOCKSIZE)) == -1) {
                perror("Error: ");
                return 1;
            }
        }
    }
   /* extract option specified */
   else if (x == 1){
      if (v == 1){
         extract(argv[2], (argc - 3), (argv + 3), 1);
      }
      else{
         extract(argv[2], (argc - 3), (argv + 3), 0);
      }
   }
   /* list option specified */
   else if (t == 1){
      if (v == 1){
         t_names(argv[2], (argc - 3), (argv + 3), 1);
      }
      else{
         t_names(argv[2], (argc - 3), (argv + 3), 0);
      }
   }

    free(options);
    free(tarfile);
    /* change */
    free(archive);
    close(fd);
    return 0;
}
