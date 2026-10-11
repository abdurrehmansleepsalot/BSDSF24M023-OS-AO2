/*
- Programming Assignment 02: lsv1.0.0

- This is the source file of version 1.0.0

- Read the write-up of the assignment to add the features to this base version

- Usage:
*       $ lsv1.0.0 
*       % lsv1.0.0  /home
*       $ lsv1.0.0  /home/kali/   /etc/
*       $ lsv1.0.0  -l /home
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

extern int errno;

// Helper function to format st_mode integer into rwxrwxrwx string
void mode_to_letters(int mode, char str[])
{
    strcpy(str, "----------");

    // File type check
    if (S_ISDIR(mode))  str[0] = 'd';
    if (S_ISCHR(mode))  str[0] = 'c';
    if (S_ISBLK(mode))  str[0] = 'b';
    if (S_ISLNK(mode))  str[0] = 'l';
    if (S_ISFIFO(mode)) str[0] = 'p';
    if (S_ISSOCK(mode)) str[0] = 's';

    // Owner permissions
    if (mode & S_IRUSR) str[1] = 'r';
    if (mode & S_IWUSR) str[2] = 'w';
    if (mode & S_IXUSR) str[3] = 'x';

    // Group permissions
    if (mode & S_IRGRP) str[4] = 'r';
    if (mode & S_IWGRP) str[5] = 'w';
    if (mode & S_IXGRP) str[6] = 'x';

    // Others permissions
    if (mode & S_IROTH) str[7] = 'r';
    if (mode & S_IWOTH) str[8] = 'w';
    if (mode & S_IXOTH) str[9] = 'x';
}

// Function to handle long listing printing
void show_file_info(const char *dir, const char *filename)
{
    struct stat info;
    char path[1024];

    snprintf(path, sizeof(path), "%s/%s", dir, filename);

    // Using lstat to properly handle symbolic links
    if (lstat(path, &info) == -1)
    {
        perror(filename);
        return;
    }

    char modestr[11];
    mode_to_letters(info.st_mode, modestr);

    struct passwd *pw = getpwuid(info.st_uid);
    struct group *gr = getgrgid(info.st_gid);

    char *uid_name = pw ? pw->pw_name : "unknown";
    char *gid_name = gr ? gr->gr_name : "unknown";

    // Formatting modification time
    char timebuf[64];
    struct tm *tm_info = localtime(&info.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm_info);

    printf("%s %2ld %-8s %-8s %8lld %s %s\n",
           modestr,
           (long)info.st_nlink,
           uid_name,
           gid_name,
           (long long)info.st_size,
           timebuf,
           filename);
}

void do_ls(const char *dir, int long_mode);

int main(int argc, char *argv[])
{
    int opt;
    int long_mode = 0;

    // Parse command line arguments for -l flag using getopt()
    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        switch (opt)
        {
        case 'l':
            long_mode = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [directory...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Process directories after options
    if (optind == argc)
    {
        do_ls(".", long_mode);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (argc - optind > 1)
            {
                printf("Directory listing of %s : \n", argv[i]);
            }
            do_ls(argv[i], long_mode);
            if (argc - optind > 1)
            {
                puts("");
            }
        }
    }
    return 0;
}

void do_ls(const char *dir, int long_mode)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (long_mode)
        {
            show_file_info(dir, entry->d_name);
        }
        else
        {
            printf("%s\n", entry->d_name);
        }
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);
}
