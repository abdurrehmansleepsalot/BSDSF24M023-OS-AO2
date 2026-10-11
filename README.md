# Analysis Report - Feature-2: Long Listing Format (ls-v1.1.0)

## Question 1: What is the crucial difference between the `stat()` and `lstat()` system calls? In the context of the `ls` command, when is it more appropriate to use `lstat()`?

### Answer:
The primary difference between `stat()` and `lstat()` lies in how they handle **symbolic links**:

- **`stat()`**: Follows (dereferences) symbolic links. When given a path pointing to a symbolic link, `stat()` returns the metadata and properties of the **target file or directory** that the link points to, rather than the link itself.
- **`lstat()`**: Does **not** follow symbolic links. When given a path pointing to a symbolic link, `lstat()` returns the metadata of the **symbolic link itself** (such as link ownership, link permissions, and link size).

### Appropriateness in `ls`:
In the context of the `ls` command, using **`lstat()`** is more appropriate because:
1. `ls` needs to accurately inspect and display information about the actual contents of a directory.
2. If `stat()` were used, a broken symbolic link (a link pointing to a non-existent file) would cause `stat()` to return an error (`ENOENT`), making the file appear unreadable or inaccessible.
3. `lstat()` allows `ls` to correctly identify symbolic link entries (e.g., displaying `l` in permissions) and retrieve their specific file details without inadvertently resolving the target destination.

---

## Question 2: The `st_mode` field in `struct stat` is an integer that contains both the file type and the permission bits. Explain how you can use bitwise operators and predefined macros to extract this information.

### Answer:
The `st_mode` field in `struct stat` is a bit mask storing file type, special bits, and access permissions combined within a single integer.

### 1. Extracting File Type:
To extract the file type, a bitwise AND (`&`) operation is performed between `st_mode` and the file type mask macro `S_IFMT`:

```c
mode_t file_type = st.st_mode & S_IFMT;
