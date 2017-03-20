#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <dirent.h>
#include <term.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <fcntl.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <curses.h>
#include <sys/wait.h>
#include <sys/types.h>

#define BUFFER_SIZE 1024
#define COPY_BUFFER 8192
#define TOKEN_SIZE 64
#define TOKEN_DELIMITER " \t\r\n\a"

extern int alphasort() ;
// Built-in routine to sort an array alphabetically

/* Function prototypes */

int sh_chdir (char **args) ;
int sh_help (char **args) ;
int sh_exit (char **args) ;
int sh_mkfile (char **args) ;
int sh_rmfile (char **args) ;
int sh_read (char **args) ;
int sh_cpfile (char **args) ;
int sh_calc (char **args) ;
int sh_write (char **args) ;
int sh_time (char **args) ;
int sh_append (char **args) ;
int sh_rename (char **args) ;
int sh_matchpat (char **args) ;
int sh_list (char **args) ;
int sh_file (char **args) ;
int sh_clrscr (char **args) ;

int launcher (char **args) ;
int execute (char **args) ;

char *read_line (void) ;
char **parse_line (char *line) ;
void shell_loop (void) ;

/* Built-in commands and their functions */

char *built_in_string[] = {
  "chdir",
  "read",
  "write",
  "append",
  "mkfile",
  "rmfile",
  "cpfile",
  "rename",
  "file",
  "matchpat",
  "list",
  "calc",
  "time",
  "clrscr",
  "help",
  "exit"
} ;

int (*built_in_function[]) (char **) = {
  &sh_chdir,
  &sh_read,
  &sh_write,
  &sh_append,
  &sh_mkfile,
  &sh_rmfile,
  &sh_cpfile,
  &sh_rename,
  &sh_file,
  &sh_matchpat,
  &sh_list,
  &sh_calc,
  &sh_time,
  &sh_clrscr,
  &sh_help,
  &sh_exit
} ;

char *built_in_string_help[] = {
  "Change the directory from the current directory.",
  "Open a file and print its content onto the console.",
  "Erase an existing file to write or create a new file and write to it.",
  "Append data to an existing file or create a new file and write to it.",
  "Create a new file (1 file at a time).",
  "Remove an existing file permanently (1 file at a time).",
  "Copy one file to another (overwrite).", 
  "Renames any given filename based on the given alternative.",
  "Gives information about the given file, like file permissions.",
  "A basic command to serve the purpose of pattern matching.",
  "Command to list all the files and folders in a given directory.",
  "Basic n-digit calculator used for computation (2 arguments at a time).",
  "Returns the current time based on the current time zone.",
  "Clears the screen buffer of the custom shell.",
  "Manual page for the custom generated shell.",
  "Terminate the shell altogether."
} ;

int number_built_in () {
  return sizeof(built_in_string)/sizeof(char *) ;
}

/* Built-in function implementations - Run until '1' is returned */

/* chdirec: change directory */

int sh_chdir (char **args) {
  if (args[1] == NULL)
    fprintf(stderr, "ERROR: Expected argument to \"chdir\".\n") ;
  else
    if (chdir(args[1]) != 0) {
      fprintf(stderr, "ERROR: \"chdir\" failed.\n") ;
      return 1 ;
    }
  return 1 ;
}

/* help: Gives details on which commands are built-in in the shell */

int sh_help (char **args) {
  printf("Operating Systems Project - Shell.\n") ;
  printf("Enter the program names and arguments and hit [Enter].\n\n") ;
  printf("The following are built-in:\n") ;

  system("tput setaf 3") ;
  for (int i = 0; i < number_built_in(); i++)
    printf("    %d. %s - %s\n", i+1, built_in_string[i], built_in_string_help[i]) ;
  system("tput sgr0") ;
  
  printf("\nUse the \"man\" page for information about other programs.\n") ;
  
  return 1 ;
}

/* exit: Returns '0' which terminates the shell */

int sh_exit (char **args) {
  return 0 ;
}

/* sh_mkfile: Creates a new file */

int sh_mkfile (char **args) {
  if (args[1] == NULL)
    fprintf(stderr, "ERROR: Filename is required.\n") ;
  else {
    int fd = creat(args[1], S_IRWXU|S_IWUSR|S_IRGRP|S_IROTH) ;
    // S_IRWXU: File owner has read, write and execute permissions
    // S_IWUSR: User has write permission 
    // S_IRGRP: Group has read permission
    // S_IROTH: Others have read permission
    
    if (fd == -1) {
      fprintf(stderr, "ERROR: File not created.\n") ;
      return 1 ;
    }
    close(fd) ;
    return 1 ;
  }
}

/* sh_rmfile: Removes a file permanently */

int sh_rmfile (char **args) {
  if (args[1] == NULL)
    fprintf(stderr, "ERROR: Filename is required.\n") ;
  else 
    if (unlink(args[1]) == -1) {
      fprintf(stderr, "ERROR: File not deleted.\n") ;
      return 1 ;
    }
  remove(args[1]) ;
  return 1 ;
}

/* sh_read: Opens a file */

int sh_read (char **args) {
  if (args[1] == NULL)
    fprintf(stderr, "ERROR: Filename is required.\n") ;
  else {
    int fd = open(args[1], O_RDONLY) ;
    ssize_t bytes ;
    char content[COPY_BUFFER] ;
    if (fd == -1) {
      fprintf(stderr, "ERROR: File cannot be opened.\n") ;
      return 1 ;
    }
    while ((bytes = read(fd, content, sizeof(content)-1)) > 0)
      write (1, content, bytes) ;
    close (fd);
  }
  return 1 ;
}

/* sh_cpfile: Copy two files */

int sh_cpfile (char **args) {
  int in_fd, out_fd ;
  ssize_t in_bytes, out_bytes ;
  char buffer[COPY_BUFFER] ;
  
  if(args[1] == NULL && args[2] == NULL) {
    fprintf(stderr, "ERROR: Two filenames are required.\n") ;
    return 1 ;
  }
  
  in_fd = open (args[1], O_RDONLY) ;
  if (in_fd == -1) {
    fprintf(stderr, "ERROR: Cannot read the first file.\n") ;
    return 1 ;
  }
  
  out_fd = open (args[2], O_WRONLY | O_CREAT, 0644) ;
  if(out_fd == -1) {
    fprintf(stderr, "ERROR: Cannot write to the second file.\n") ;
    return 1 ;
  }
  
  while ((in_bytes = read(in_fd, buffer, sizeof(buffer)-1)) > 0) {
    out_bytes = write (out_fd, buffer, in_bytes) ;
    if(out_bytes != in_bytes) {
      fprintf(stderr, "ERROR: Write error.\n") ;
      return 1 ;
    }
  }
  close (in_fd) ;
  close (out_fd) ;
  return 1 ;
}

/* sh_calc: Calculator */

int sh_calc (char **args) {
  if(args[1] == NULL && args[2] == NULL && args[3] == NULL) {
    fprintf(stderr, "ERROR: Invalid arguments.\n") ;
    return 1 ;
  }
  char expr[128] ;
  sprintf(expr, "echo %s%s%s | bc", args[1], args[2], args[3]) ;
  system(expr) ;
  return 1 ;
}

/* sh_write: Write to a new file or erase an existing file and write to it */

int sh_write (char **args) {
  if (args[1] == NULL)
    fprintf(stderr, "ERROR: Filename is required.\n") ;
  else {
    int fd = open(args[1], O_WRONLY | O_CREAT | O_TRUNC, 0644) ;
    char content[COPY_BUFFER] ;
    if (fd == -1) {
      fprintf(stderr, "ERROR: File cannot be opened (or) \"%s\" is a directory/ an executable.\n", args[1]) ;
      return 1 ;
    }
    ssize_t bytes ;
    while ((bytes = read(0, content, sizeof(content)-1)) > 0) 
      write(fd, content, bytes) ;
    close (fd);
  }
  return 1 ;
}

/* sh_time: Give all the details about current time */

int sh_time (char **args) {
  time_t raw_time ;
  struct tm *time_info ;
  time (&raw_time) ;
  time_info = localtime (&raw_time) ;
  printf("%s", asctime(time_info)) ;
  // asctime: Returns a string with format - Www Mmm dd hh:mm:ss yyyy 
  return 1 ;
}

/* sh_append: Append data to a file (no overwriting) */

int sh_append (char **args) {
  if (args[1] == NULL)
    fprintf(stderr, "ERROR: Filename is required.\n") ;
  else {
    int fd = open(args[1], O_RDWR | O_APPEND | O_CREAT, 0644) ;
    char content[COPY_BUFFER] ;
    if (fd == -1) {
      fprintf(stderr, "ERROR: File cannot be opened.\n") ;
      return 1 ;
    }
    ssize_t bytes ;
    while ((bytes = read(0, content, sizeof(content)-1)) > 0) 
      write(fd, content, bytes) ;
    close (fd);
  }
  return 1 ;
}

/* sh_rename: Renames a given file based on the alternative */

int sh_rename (char **args) {
  if (args[1] == NULL && args[2] == NULL)
    fprintf(stderr, "ERROR: Two arguments required.\n") ;
  else
    if (rename(args[1], args[2]) == -1) {
      fprintf(stderr, "ERROR: Failed to rename.\n") ;
      return 1 ;
    }
  return 1 ;
}

/* sh_matchpat: Matches patterns in files */

int sh_matchpat (char **args) {
  if (args[1] == NULL && args[2] == NULL)
    fprintf(stderr, "ERROR: Invalid number of arguments.\n") ;
  else {
    int fd = open(args[2], O_RDONLY), position = 0 ;
    char content, line[COPY_BUFFER] ;
    memset (line, 0, sizeof(line)) ;
    if (fd == -1) {
      fprintf(stderr, "ERROR: Cannot read the file") ;
      return 1 ;
    }
    ssize_t bytes ;
    while ((bytes = read(fd, &content, sizeof(char))) > 0) {
      if (content != '\n') {
	line[position] = content ;
	position++ ;
      } else {
	if (strstr(line, args[1]) != NULL)
	  printf("%s\n", line) ;
	memset (line, 0, sizeof(line)) ;
	position = 0 ;
      }
    }
    return 1 ;
  }
}

/* sh_list: List files and folders in a given directory */

/* file_select: Selects specific files (leaves out '.', '..', '.*') */

static int file_select (const struct direct *entry) {
  char name[COPY_BUFFER] ;
  if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
    return 0 ;
  else {
    // index(string s, char c): First occurence of 'c' in 's'
    // rindex(string s, char c): Last occurence of 'c' in 's'
    strcpy (name, entry->d_name) ;
    if (name[0] == '.')
      return 0 ;
    return 1 ;
  }
}

int sh_list (char **args) {
  struct direct **files ;
  char pathname[MAXPATHLEN] ;     // MAXPATHLEN: sys/param.h
  if (getcwd(pathname, sizeof(pathname)) == NULL) {
    fprintf(stderr, "ERROR: Cannot get the path.\n") ;
    return 1 ;
  }
  int count = scandir(pathname, &files, file_select, alphasort) ;
  // int count = scandir(pathname, &files, 0, alphasort) ;
  
  if (count < 1) {
    fprintf(stderr, "ERROR: No files found in this directory.\n") ;
    return 1 ;
  }
  struct stat file_stat ;
  system("tput setaf 3") ;
  printf("Total files in %s: %d\n", pathname, count) ;
  system("tput sgr0") ;
  for(int i = 0; i < count; i++) {
    stat(files[i]->d_name, &file_stat) ;
    if (S_ISDIR(file_stat.st_mode)) {    // Directory
      system("tput setaf 2") ;
      system("tput bold") ;
    } else if (file_stat.st_mode & S_IXUSR) {    // Executable
      system("tput setaf 6") ;
      system("tput bold") ;
    }
    printf("%s\n", files[i]->d_name) ;
    system("tput sgr0") ;
    // scandir: Only allocates memory and doesnot deallocate
    free(files[i]) ;
  }
  return 1 ;
}

/* sh_file: Gives information about the file */

int sh_file (char **args) {
  if (args[1] == NULL)
    fprintf(stderr, "ERROR: Filename necessary.\n") ;
  else {
    struct stat file_stat ;
    if (stat(args[1], &file_stat) < 0) {
      fprintf(stderr, "ERROR: File statistics couldnot be loaded.\n") ;
      return 1 ;
    }
    system("tput setaf 3") ;
    printf("Information about %s:\n", args[1]) ;
    system("tput sgr0") ;
    printf("File Size: \t\t%ld bytes.\n", file_stat.st_size) ;
    printf("Number of Links: \t%ld.\n", file_stat.st_nlink) ;
    printf("File inode: \t\t%ld.\n", file_stat.st_ino) ;

    printf("File Permissions: \t") ;
    printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-") ;
    printf((file_stat.st_mode & S_IRUSR) ? "r" : "-") ;
    printf((file_stat.st_mode & S_IWUSR) ? "w" : "-") ;
    printf((file_stat.st_mode & S_IXUSR) ? "x" : "-") ;
    printf((file_stat.st_mode & S_IRGRP) ? "r" : "-") ;
    printf((file_stat.st_mode & S_IWGRP) ? "w" : "-") ;
    printf((file_stat.st_mode & S_IXGRP) ? "x" : "-") ;
    printf((file_stat.st_mode & S_IROTH) ? "r" : "-") ;
    printf((file_stat.st_mode & S_IWOTH) ? "w" : "-") ;
    printf((file_stat.st_mode & S_IXOTH) ? "x" : "-") ;
    printf("\n\n");
 
    printf("The file \'%s\' a symbolic link.\n", (S_ISLNK(file_stat.st_mode)) ? "is" : "is not");
  }
  return 1 ; 
}

/* sh_clrscr: Clear the screen buffer */

int sh_clrscr (char **args) {
  system("clear") ;
  return 1 ;
}

/* launcher: Launches the custom shell */

int launcher (char **args) {
  int status ;
  pid_t pid = fork() ;
  
  if (pid == 0) {
    /* In the child process */
    if (execvp(*args, args) == -1) {
      fprintf(stderr, "ERROR: Unable to execute.\n") ;
      exit(1) ;
    }
  } else if (pid > 0) {
    /* In parent process */
    while (wait(&status) != pid) ;     // wait for completion
  } else
    fprintf(stderr, "ERROR: Cannot fork the child process.\n") ;
  
  return 1 ;
}

/* execute: Executes the built-in commands and programs using launcher */

int execute (char **args) {
  if (args[0] == NULL)
    return 1 ;

  for (int i = 0; i < number_built_in (); i++)
    if (strcmp(args[0], built_in_string[i]) == 0)
      return (*built_in_function[i]) (args) ;
  
  return launcher (args) ;
}

/* read_line: Used to read a line, character by character */

char *read_line (void) {
  int buffer_size = BUFFER_SIZE, position = 0 ;
  char *buffer = malloc(sizeof(char) * buffer_size) ;

  if (!buffer) {
    fprintf(stderr, "ERROR: Memory allocation error.\n") ;
    exit(1) ;
  }

  while(1) { 
    char ch = getchar() ;    // Read a single character
    
    if (ch == EOF || ch == '\n') {
      buffer[position] = '\0' ;    // Add a null character in the end
      return buffer ;
    } else
      buffer[position] = ch ;
    position++ ;

    // If buffer length is exceeded - Increase the buffer size
    if (position >= buffer_size) {
      buffer_size += BUFFER_SIZE ;    // Add another 1024 bits
      buffer = realloc(buffer, buffer_size) ;

      if(!buffer) {
	fprintf(stderr, "ERROR: Memory allocation error.\n") ;
	exit(1) ;
      }
    }
  }
}

/* parse_line: Used to parse the line to get arguments */

char **parse_line (char *line) {
  int buffer_size = TOKEN_SIZE, position = 0 ;    // Split into 64 bits each
  char *token, **tokens = malloc(sizeof(char *) * buffer_size) ;
  
  if(!tokens) {
    fprintf(stderr, "ERROR: Token memory allocation error.\n") ;
    exit(1) ;
  }

  token = strtok (line, TOKEN_DELIMITER) ;    // Splits line into tokens
  while (token != NULL) {
    tokens[position] = token ;
    position++ ;

    // If buffer length is exceeded - Increase the buffer size
    if (position >= buffer_size) {
      buffer_size += TOKEN_SIZE ;
      tokens = realloc(tokens, buffer_size * sizeof(char *)) ;
      
      if(!tokens) {
	fprintf(stderr, "ERROR: Token memory allocation error.\n") ;
	exit(1) ;
      }
    }
    
    token = strtok (NULL, TOKEN_DELIMITER) ;
  }
  tokens[position] = NULL ;
  return tokens ;
}

/* shell_loop: Runs the whole shell until terminated */

void shell_loop (void) {
  char *line, **args ;
  int status ;

  do {
    system("tput setaf 4") ;
    system("tput bold") ;
    system("echo -n \"$PWD >> \"") ;
    system("tput sgr0") ;
    line = read_line () ;
    args = parse_line (line) ;
    status = execute (args) ;

    free (line) ;
    free (args) ;
  } while (status) ;
}

/* main: Starts up the shell loop */

int main (int argc, int *argv[]) {
  system("clear") ;
  system("tput dim") ;
  printf("Operating Systems Project - The shell is written in C language using POSIX System Calls.\n") ;
  printf("Usage of scripting in any form is not done in the project.\n") ;
  printf("This shell does not make use of any existing shell. Use \"help\" command to view the built-ins.\n\n") ;
  system("tput sgr0") ;
  shell_loop () ;
  system("clear") ;
  return 0 ;
}
