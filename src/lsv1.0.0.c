/*
- Programming Assignment 02: lsv1.0.0

- Features:
*       -l option: Long listing format (sorted)
*       -x option: Horizontal column display (sorted)
*       Default option: Column Display (Down Then Across, sorted)
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

// State Management for Display Modes
#define MODE_DEFAULT 0
#define MODE_LONG    1
#define MODE_HORIZ   2

// Task 2: Comparison function for qsort()
int compare_filenames(const void *a, const void *b)
{
    // Cast generic const void * pointers to const char ** (pointer to string)
    const char *fileA = *(const char **)a;
    const char *fileB = *(const char **)b;
    return strcmp(fileA, fileB);
}

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

// Function to handle long listing printing (-l)
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

// Vertical Column Display ("Down Then Across")
void print_column_format(char **files, int count, int max_len)
{
    if (count == 0)
        return;

    struct winsize ws;
    int term_width = 80;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
    {
        term_width = ws.ws_col;
    }

    int spacing = 2;
    int col_width = max_len + spacing;
    int cols = term_width / col_width;

    if (cols < 1)
        cols = 1;

    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int index = c * rows + r;
            if (index < count)
            {
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

// Horizontal Column Display ("Across Then Down" -x option)
void print_horizontal_format(char **files, int count, int max_len)
{
    if (count == 0)
        return;

    struct winsize ws;
    int term_width = 80;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
    {
        term_width = ws.ws_col;
    }

    int spacing = 2;
    int col_width = max_len + spacing;
    int cols = term_width / col_width;

    if (cols < 1)
        cols = 1;

    for (int i = 0; i < count; i++)
    {
        if (i > 0 && i % cols == 0)
        {
            printf("\n");
        }

        if ((i + 1) % cols == 0 || i == count - 1)
        {
            printf("%s", files[i]);
        }
        else
        {
            printf("%-*s", col_width, files[i]);
        }
    }
    printf("\n");
}

void do_ls(const char *dir, int display_mode);

int main(int argc, char *argv[])
{
    int opt;
    int display_mode = MODE_DEFAULT;

    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        switch (opt)
        {
        case 'l':
            display_mode = MODE_LONG;
            break;
        case 'x':
            display_mode = MODE_HORIZ;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [-x] [directory...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        do_ls(".", display_mode);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (argc - optind > 1)
            {
                printf("Directory listing of %s : \n", argv[i]);
            }
            do_ls(argv[i], display_mode);
            if (argc - optind > 1)
            {
                puts("");
            }
        }
    }
    return 0;
}

void do_ls(const char *dir, int display_mode)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    int capacity = 16;
    int count = 0;
    int max_len = 0;
    char **files = malloc(capacity * sizeof(char *));

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

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

    if (errno != 0)
    {
        perror("readdir failed");
    }

    // Task 2: Sort all collected filenames alphabetically
    qsort(files, count, sizeof(char *), compare_filenames);

    // Task 3: Render pre-sorted list based on selected display mode
    if (display_mode == MODE_LONG)
    {
        for (int i = 0; i < count; i++)
        {
            show_file_info(dir, files[i]);
        }
    }
    else if (display_mode == MODE_HORIZ)
    {
        print_horizontal_format(files, count, max_len);
    }
    else if (display_mode == MODE_DEFAULT)
    {
        print_column_format(files, count, max_len);
    }

    // Free allocated memory
    for (int i = 0; i < count; i++)
    {
        free(files[i]);
    }
    free(files);
    closedir(dp);
}
