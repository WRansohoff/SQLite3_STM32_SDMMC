/**
 * Minimal implementation of the OS system calls expected by
 * the sqlite3 database library. Intended for use with STM32L4
 * targets through the SD/MMC interface. This implementation is
 * very bare-bones and may be missing key features.
 *
 * MIT License
 * Copyright (c) 2019 WRansohoff
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef VVC_ARM_SQLITE_SYSCALLS
#define VVC_ARM_SQLITE_SYSCALLS

// Include core typedefs.
#include <sys/types.h>

// `stat.h` macros and structs.
struct stat {
  dev_t     st_dev;
  ino_t     st_ino;
  mode_t    st_mode;
  nlink_t   st_nlink;
  uid_t     st_uid;
  gid_t     st_gid;
  dev_t     st_rdev;
  off_t     st_size;
  time_t    st_atime;
  long      st_spare1;
  time_t    st_mtime;
  long      st_spare2;
  time_t    st_ctime;
  long      st_spare3;
  blksize_t st_blksize;
  blkcnt_t  st_blocks;
  long      st_spare4[ 2 ];
};

#define    S_BLKSIZE  512 /* size of a block */
#define    _IFMT      0170000  /* type of file */
#define    _IFDIR     0040000  /* directory */
#define    _IFCHR     0020000  /* character special */
#define    _IFBLK     0060000  /* block special */
#define    _IFREG     0100000  /* regular */
#define    _IFLNK     0120000  /* symbolic link */
#define    _IFSOCK    0140000  /* socket */
#define    _IFIFO     0010000  /* fifo */
#define    S_IFMT     _IFMT
#define    S_IFDIR    _IFDIR
#define    S_IFCHR    _IFCHR
#define    S_IFBLK    _IFBLK
#define    S_IFREG    _IFREG
#define    S_IFLNK    _IFLNK
#define    S_IFSOCK   _IFSOCK
#define    S_IFIFO    _IFIFO
#define    S_IRWXU    ( S_IRUSR | S_IWUSR | S_IXUSR )
#define    S_IRUSR    0000400 /* read permission, owner */
#define    S_IWUSR    0000200 /* write permission, owner */
#define    S_IXUSR    0000100 /* execute/search permission, owner */
#define    S_IRWXG    ( S_IRGRP | S_IWGRP | S_IXGRP )
#define    S_IRGRP    0000040 /* read permission, group */
#define    S_IWGRP    0000020 /* write permission, grougroup */
#define    S_IXGRP    0000010 /* execute/search permission, group */
#define    S_IRWXO    ( S_IROTH | S_IWOTH | S_IXOTH )
#define    S_IROTH    0000004 /* read permission, other */
#define    S_IWOTH    0000002 /* write permission, other */
#define    S_IXOTH    0000001 /* execute/search permission, other */
// Macros to check if a file/directory matches a format flag.
#define  S_ISBLK( m )  ( ( ( m )&_IFMT ) == _IFBLK )
#define  S_ISCHR( m )  ( ( ( m )&_IFMT ) == _IFCHR )
#define  S_ISDIR( m )  ( ( ( m )&_IFMT ) == _IFDIR )
#define  S_ISFIFO( m ) ( ( ( m )&_IFMT ) == _IFIFO )
#define  S_ISREG( m)   ( ( ( m )&_IFMT ) == _IFREG )
#define  S_ISLNK( m )  ( ( ( m )&_IFMT ) == _IFLNK )
#define  S_ISSOCK( m ) ( ( ( m )&_IFMT ) == _IFSOCK )

// Required syscall method declarations.
// Close a previously-opened file.
int close( int __fd );
// Check whether the calling process can access a file.
int access( const char *__path, int __mode );
// Get the current working directory of the calling process
// as a null-terminated C-string.
char* getcwd( char *__buf, size_t __size );
// Read information about a given file into a `stat` struct.
int stat ( const char *__path,
           struct stat *__sbuf );
// Perform `stat` on a file, given its file descriptor
// instead of a string path.
int fstat( int __fd, struct stat *__sbuf );
// Truncate the given file to a given length. If the file was
// longer, extra data is lost. If it was shorter, it is padded
// with zeroes. The file must already be open for writing.
int ftruncate( int __fd, off_t __length );
// Function which manipulates a file descriptor in a variety of ways.
int fnctl( int __fd, int __cmd, ... );
// Function to read N bytes from a file.
ssize_t read( int __fd, void *__buf, size_t __count );
// Function to write N bytes to a file.
ssize_t write( int __fd, const void *__buf, size_t __count );
// Change the access permissions on a file.
int fchmod( int __fd, mode_t __mode );
// Delete a filename. If it is the last link to a given file,
// also delete that file.
int unlink( const char *__path );
// Create a new directory.
int mkdir( const char *__path, mode_t __mode );
// Remove a directory.
int rmdir( const char *__path );
// Read the contents of a symbolic link.
ssize_t readlink( const char *__path, char *__buf, size_t __bufsiz );
// Perform `stat` on a file, but if the target is a symbolic
// link, `stat` the link instead of the file it refers to.
// Currently this just wraps `stat`; I do not plan to use symlinks.
int lstat( const char *__path,
           struct stat *__buf );

#endif
