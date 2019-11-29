#include "syscalls.h"

// Open a file and return a file descriptor ID.
int open( const char *__path, int __flags, mode_t __mode ) {
  // TODO
  return -1;
}

// Close a previously-opened file.
int close( int __fd ) {
  // TODO
  return 0;
}

// Check whether the calling process can access a file.
int access( const char *__path, int __mode ) {
  // TODO
  return -1;
}

// Get the current working directory of the calling process
// as a null-terminated C-string.
char* getcwd( char *__buf, size_t __size ) {
  // TODO
  if ( __size > 0 ) { __buf[ 0 ] = '\0'; }
  return NULL;
}

// Read information about a given file into a `stat` struct.
int stat ( const char *__path,
           struct stat *__sbuf ) {
  // TODO
  return -1;
}

// Perform `stat` on a file, given its file descriptor
// instead of a string path.
int fstat( int __fd, struct stat *__sbuf ) {
  // TODO
  return -1;
}

// Truncate the given file to a given length. If the file was
// longer, extra data is lost. If it was shorter, it is padded
// with zeroes. The file must already be open for writing.
int ftruncate( int __fd, off_t __length ) {
  // TODO
  return -1;
}

// Function which manipulates a file descriptor in a variety of ways.
int fnctl( int __fd, int __cmd, ... ) {
  // TODO
  return -1;
}

// Function to read N bytes from a file.
ssize_t read( int __fd, void *__buf, size_t __count ) {
  // TODO
  return 0;
}

// Function to write N bytes to a file.
ssize_t write( int __fd, const void *__buf, size_t __count ) {
  // TODO
  return 0;
}

// Change the access permissions on a file.
int fchmod( int __fd, mode_t __mode ) {
  // TODO
  return -1;
}

// Delete a filename. If it is the last link to a given file,
// also delete that file.
int unlink( const char *__path ) {
  // TODO
  return -1;
}

// Create a new directory.
int mkdir( const char *__path, mode_t __mode ) {
  // TODO
  return -1;
}

// Remove a directory.
int rmdir( const char *__path ) {
  // TODO
  return -1;
}

// Read the contents of a symbolic link.
ssize_t readlink( const char *__path, char *__buf, size_t __bufsiz ) {
  // TODO
  if ( __bufsiz > 0 ) { __buf[ 0 ] = '\0'; }
  return 0;
}

// Perform `stat` on a file, but if the target is a symbolic
// link, `stat` the link instead of the file it refers to.
// Currently this just wraps `stat`; I do not plan to use symlinks.
int lstat( const char *__path,
           struct stat *__buf ) {
  return stat( __path, __buf );
}

// Perform bare-metal initialization.
int sqlite3_os_init( void ) {
  // TODO
  return -1;
}

// Perform bare-metal shutdown.
int sqlite3_os_end( void ) {
  // TODO
  return -1;
}
