/*
- Programming Assignment 02: lsv1.0.0

- Features:
*       -l option: Long listing format
*       Default option: Column Display (Down Then Across) using ioctl
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
#include <sys/ioctl.h>

extern int errno;

// Helper function to format st_mode integer into rwxrwxrwx string
void mode_to_letters(int mode, char str[])
{
    strcpy(str, "----------");

    if (S_ISDIR(mode))  str[0] = 'd';
    if (S_ISCHR(mode))  str[0] = 'c';
    if (S_ISBLK(mode))  str[0] = 'b';
    if (S_ISLNK(mode))  str[0] = 'l';
    if (S_ISFIFO(mode)) str[0] = 'p';
    if (S_ISSOCK(mode)) str[0] = 's';

    if (mode & S_IRUSR) str[1] = 'r';
    if (mode & S_IWUSR) str[2] = 'w';
    if (mode & S_IXUSR) str[3] = 'x';

    if (mode & S_IRGRP) str[4] = 'r';
    if (mode & S_IWGRP) str[5] = 'w';
    if (mode & S_IXGRP) str[6] = 'x';

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

// Task 3 & 4: Down then across column printing function
void print_column_format(char **files, int count, int max_len)
{
    if (count == 0)
        return;

    // Task 3: Determine Terminal Width using ioctl (TIOCGWINSZ)
    struct winsize ws;
    int term_width = 80; // Fallback width

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
    {
        term_width = ws.ws_col;
    }

    int spacing = 2; // Space between columns
    int col_width = max_len + spacing;
    int cols = term_width / col_width;

    if (cols < 1)
        cols = 1;

    int rows = (count + cols - 1) / cols;

    // Task 4: Iterate row by row for "down then across" output
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int index = c * rows + r;
            if (index < count)
            {
                // Pad spaces to align with the next column
                if (c == cols - 1 || (c + 1) * rows + r >= count)
                {
                    printf("%s", files[index]);
                }
                else
                {
                    printf("%-*s", col_width, files[index]);
                }
            }
        }
        printf("\n");
    }
}

void do_ls(const char *dir, int long_mode);

int main(int argc, char *argv[])
{
    int opt;
    int long_mode = 0;

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

    // Task 2: Dynamic allocation setup for filenames
    int capacity = 16;
    int count = 0;
    int max_len = 0;
    char **files = malloc(capacity * sizeof(char *));

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
            if (count >= capacity)
            {
                capacity *= 2;
                files = realloc(files, capacity * sizeof(char *));
            }
            files[count] = strdup(entry->d_name);
            int len = strlen(entry->d_name);
            if (len > max_len)
            {
                max_len = len;
            }
            count++;
        }
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    // Print default column format if -l is not passed
    if (!long_mode)
    {
        print_column_format(files, count, max_len);

        // Free dynamically allocated memory
        for (int i = 0; i < count; i++)
        {
            free(files[i]);
        }
    }

    free(files);
    closedir(dp);
}
