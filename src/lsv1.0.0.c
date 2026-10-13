/*
- Programming Assignment 02: lsv1.0.0

- Features:
*       -l option: Long listing format (sorted, colorized)
*       -x option: Horizontal column display (sorted, colorized)
*       -R option: Recursive directory listing
*       Default option: Column Display (Down Then Across, sorted, colorized)
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

// Display Modes
#define MODE_DEFAULT 0
#define MODE_LONG    1
#define MODE_HORIZ   2

// ANSI Escape Codes for File Types
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[1;34m" // Directory
#define COLOR_GREEN   "\033[1;32m" // Executable
#define COLOR_RED     "\033[1;31m" // Tarballs / Archives
#define COLOR_PINK    "\033[1;35m" // Symbolic Links
#define COLOR_REVERSE "\033[7m"    // Special Files

// Comparison function for qsort()
int compare_filenames(const void *a, const void *b)
{
    const char *fileA = *(const char **)a;
    const char *fileB = *(const char **)b;
    return strcmp(fileA, fileB);
}

// Helper to identify tarballs and compressed archives
int is_tarball(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if (!ext) return 0;

    if (strcmp(ext, ".tar") == 0 || strcmp(ext, ".gz") == 0 || 
        strcmp(ext, ".zip") == 0 || strcmp(ext, ".bz2") == 0 || 
        strcmp(ext, ".xz") == 0)
    {
        return 1;
    }
    return 0;
}

// Determine Color Code based on st_mode and file properties
const char *get_color_code(const char *dir, const char *filename)
{
    struct stat st;
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, filename);

    if (lstat(path, &st) == -1)
    {
        return COLOR_RESET;
    }

    if (S_ISLNK(st.st_mode))
    {
        return COLOR_PINK;
    }
    if (S_ISDIR(st.st_mode))
    {
        return COLOR_BLUE;
    }
    if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) || 
        S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode))
    {
        return COLOR_REVERSE;
    }
    if (is_tarball(filename))
    {
        return COLOR_RED;
    }
    if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
    {
        return COLOR_GREEN;
    }

    return COLOR_RESET;
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

// Long listing function (-l) with color
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

    const char *color = get_color_code(dir, filename);

    printf("%s %2ld %-8s %-8s %8lld %s %s%s%s\n",
           modestr,
           (long)info.st_nlink,
           uid_name,
           gid_name,
           (long long)info.st_size,
           timebuf,
           color,
           filename,
           COLOR_RESET);
}

// Vertical Column Display ("Down Then Across")
void print_column_format(const char *dir, char **files, int count, int max_len)
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
                const char *color = get_color_code(dir, files[index]);
                printf("%s%s%s", color, files[index], COLOR_RESET);

                if (!(c == cols - 1 || (c + 1) * rows + r >= count))
                {
                    int pad = col_width - (int)strlen(files[index]);
                    for (int k = 0; k < pad; k++)
                    {
                        putchar(' ');
                    }
                }
            }
        }
        printf("\n");
    }
}

// Horizontal Column Display ("Across Then Down")
void print_horizontal_format(const char *dir, char **files, int count, int max_len)
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

        const char *color = get_color_code(dir, files[i]);
        printf("%s%s%s", color, files[i], COLOR_RESET);

        if (!((i + 1) % cols == 0 || i == count - 1))
        {
            int pad = col_width - (int)strlen(files[i]);
            for (int k = 0; k < pad; k++)
            {
                putchar(' ');
            }
        }
    }
    printf("\n");
}

void do_ls(const char *dir, int display_mode, int recursive_mode);

int main(int argc, char *argv[])
{
    int opt;
    int display_mode = MODE_DEFAULT;
    int recursive_mode = 0;

    // Task 2: Extend getopt() for -l, -x, and -R flags
    while ((opt = getopt(argc, argv, "lxR")) != -1)
    {
        switch (opt)
        {
        case 'l':
            display_mode = MODE_LONG;
            break;
        case 'x':
            display_mode = MODE_HORIZ;
            break;
        case 'R':
            recursive_mode = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [-x] [-R] [directory...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        do_ls(".", display_mode, recursive_mode);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (argc - optind > 1 || recursive_mode)
            {
                printf("%s:\n", argv[i]);
            }
            do_ls(argv[i], display_mode, recursive_mode);
            if (i < argc - 1)
            {
                puts("");
            }
        }
    }
    return 0;
}

// Task 3: Modified do_ls Function with Recursive Support
void do_ls(const char *dir, int display_mode, int recursive_mode)
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

    closedir(dp);

    // Sort filenames alphabetically
    qsort(files, count, sizeof(char *), compare_filenames);

    // Render sorted entries for current directory
    if (display_mode == MODE_LONG)
    {
        for (int i = 0; i < count; i++)
        {
            show_file_info(dir, files[i]);
        }
    }
    else if (display_mode == MODE_HORIZ)
    {
        print_horizontal_format(dir, files, count, max_len);
    }
    else if (display_mode == MODE_DEFAULT)
    {
        print_column_format(dir, files, count, max_len);
    }

    // Task 3: Handle Recursive Listing (-R)
    if (recursive_mode)
    {
        for (int i = 0; i < count; i++)
        {
            char fullpath[2048];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, files[i]);

            struct stat st;
            if (lstat(fullpath, &st) == 0 && S_ISDIR(st.st_mode))
            {
                printf("\n%s:\n", fullpath);
                do_ls(fullpath, display_mode, recursive_mode);
            }
        }
    }

    // Free memory
    for (int i = 0; i < count; i++)
    {
        free(files[i]);
    }
    free(files);
}
