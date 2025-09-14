// Shell.

#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// Parsed command representation
#define EXEC  1
#define REDIR 2
#define PIPE  3
#define LIST  4
#define BACK  5

#define MAXARGS 10
#define MAX_HISTORY 20
#define MAX_LINE_LENGTH 200

struct cmd {
  int type;
};

struct execcmd {
  int type;
  char *argv[MAXARGS];
  char *eargv[MAXARGS];
};

struct redircmd {
  int type;
  struct cmd *cmd;
  char *file;
  char *efile;
  int mode;
  int fd;
};

struct pipecmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct listcmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct backcmd {
  int type;
  struct cmd *cmd;
};

// Command history structure
struct history_entry {
  char command[MAX_LINE_LENGTH];
  int length;
};

// Global history variables
static struct history_entry history[MAX_HISTORY];
static int history_count = 0;
static int history_index = -1;  // -1 means no history selected
static int history_start = 0;   // For circular buffer

int fork1(void);  // Fork but panics on failure.
void panic(char*);
struct cmd *parsecmd(char*);
void runcmd(struct cmd*) __attribute__((noreturn));

// History management functions
void add_to_history(char *cmd);
void display_history(void);
char* get_history_command(int direction);
int atoi_simple(char *str);
int strncmp_simple(char *s1, char *s2, int n);
int getchar_simple(void);

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2];
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit(1);

  switch(cmd->type){
  default:
    panic("runcmd");

  case EXEC:
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit(1);
    exec(ecmd->argv[0], ecmd->argv);
    fprintf(2, "exec %s failed\n", ecmd->argv[0]);
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    close(rcmd->fd);
    if(open(rcmd->file, rcmd->mode) < 0){
      fprintf(2, "open %s failed\n", rcmd->file);
      exit(1);
    }
    runcmd(rcmd->cmd);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    if(fork1() == 0)
      runcmd(lcmd->left);
    wait(0);
    runcmd(lcmd->right);
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    if(pipe(p) < 0)
      panic("pipe");
    if(fork1() == 0){
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if(fork1() == 0){
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait(0);
    wait(0);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    if(fork1() == 0)
      runcmd(bcmd->cmd);
    break;
  }
  exit(0);
}

int
getcmd(char *buf, int nbuf)
{
  write(2, "$ ", 2);
  memset(buf, 0, nbuf);
  
  int pos = 0;
  int c;
  
  while(1) {
    c = getchar_simple();
    if(c == '\n' || c == '\r') {
      buf[pos] = '\n';
      buf[pos + 1] = 0;
      break;
    } else if(c == 4) { // Ctrl+D (EOF)
      if(pos == 0) {
        return -1;
      }
    } else if(c == 127 || c == '\b') { // Backspace
      if(pos > 0) {
        pos--;
        write(2, "\b \b", 3); // Move back, space, move back
      }
    } else if(c == 27) { // Escape sequence (arrow keys)
      // Read the next two characters to determine which arrow key
      int c2 = getchar_simple();
      int c3 = getchar_simple();
      
      if(c2 == '[') {
        if(c3 == 'A') { // Up arrow - show history command
          char *hist_cmd = get_history_command(1);
          if(hist_cmd) {
            // Show the command on a new line for clarity
            printf("\nPrevious command: %s", hist_cmd);
            write(2, "$ ", 2);
            
            // Copy history command to buffer
            int len = strlen(hist_cmd);
            if(len > 0 && hist_cmd[len-1] == '\n') {
              strcpy(buf, hist_cmd);
              pos = len-1;
            } else {
              strcpy(buf, hist_cmd);
              pos = len;
            }
            write(2, buf, pos);
          }
        } else if(c3 == 'B') { // Down arrow - show next history command
          char *hist_cmd = get_history_command(-1);
          if(hist_cmd) {
            printf("\nNext command: %s", hist_cmd);
            write(2, "$ ", 2);
            
            // Copy history command to buffer
            int len = strlen(hist_cmd);
            if(len > 0 && hist_cmd[len-1] == '\n') {
              strcpy(buf, hist_cmd);
              pos = len-1;
            } else {
              strcpy(buf, hist_cmd);
              pos = len;
            }
            write(2, buf, pos);
          } else {
            // Clear current line completely
            printf("\nNo more history\n");
            write(2, "$ ", 2);
            memset(buf, 0, nbuf);
            pos = 0;
          }
        }
      }
    } else if(pos < nbuf - 1) {
      buf[pos] = c;
      pos++;
      write(2, &c, 1);
    }
  }
  
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

int
main(void)
{
  static char buf[100];
  int fd;

  // Ensure that three file descriptors are open.
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    char *cmd = buf;
    while (*cmd == ' ' || *cmd == '\t')
      cmd++;
    if (*cmd == '\n') // is a blank command
      continue;
      
    // Handle built-in history commands
    if(strcmp(cmd, "history\n") == 0 || strcmp(cmd, "hist\n") == 0) {
      display_history();
      continue;
    }
    
    // Handle history selection (e.g., "!1", "!2", etc.)
    if(cmd[0] == '!' && cmd[1] >= '1' && cmd[1] <= '9') {
      int hist_num = cmd[1] - '0';
      if(hist_num <= history_count) {
        int index = (history_start + hist_num - 1) % MAX_HISTORY;
        char *selected_cmd = history[index].command;
        printf("Executing: %s", selected_cmd);
        
        // Process the selected command
        if(selected_cmd[0] == 'c' && selected_cmd[1] == 'd' && selected_cmd[2] == ' ') {
          selected_cmd[strlen(selected_cmd)-1] = 0;  // chop \n
          if(chdir(selected_cmd+3) < 0)
            fprintf(2, "cannot cd %s\n", selected_cmd+3);
        } else {
          if(fork1() == 0)
            runcmd(parsecmd(selected_cmd));
          wait(0);
        }
      } else {
        printf("History entry %d not found\n", hist_num);
      }
      continue;
    }
    
    // Add command to history (before processing)
    add_to_history(cmd);
    
    if(cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == ' '){
      // Chdir must be called by the parent, not the child.
      cmd[strlen(cmd)-1] = 0;  // chop \n
      if(chdir(cmd+3) < 0)
        fprintf(2, "cannot cd %s\n", cmd+3);
    } else {
      if(fork1() == 0)
        runcmd(parsecmd(cmd));
      wait(0);
    }
  }
  exit(0);
}

void
panic(char *s)
{
  fprintf(2, "%s\n", s);
  exit(1);
}

int
fork1(void)
{
  int pid;

  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

//PAGEBREAK!
// Constructors

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = LIST;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
backcmd(struct cmd *subcmd)
{
  struct backcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = BACK;
  cmd->cmd = subcmd;
  return (struct cmd*)cmd;
}
//PAGEBREAK!
// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if(*s == '>'){
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;

  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *nulterminate(struct cmd*);

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(2, "leftovers: %s\n", s);
    panic("syntax");
  }
  nulterminate(cmd);
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parsepipe(ps, es);
  while(peek(ps, es, "&")){
    gettoken(ps, es, 0, 0);
    cmd = backcmd(cmd);
  }
  if(peek(ps, es, ";")){
    gettoken(ps, es, 0, 0);
    cmd = listcmd(cmd, parseline(ps, es));
  }
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a')
      panic("missing file for redirection");
    switch(tok){
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
      break;
    case '>':
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE|O_TRUNC, 1);
      break;
    case '+':  // >>
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    }
  }
  return cmd;
}

struct cmd*
parseblock(char **ps, char *es)
{
  struct cmd *cmd;

  if(!peek(ps, es, "("))
    panic("parseblock");
  gettoken(ps, es, 0, 0);
  cmd = parseline(ps, es);
  if(!peek(ps, es, ")"))
    panic("syntax - missing )");
  gettoken(ps, es, 0, 0);
  cmd = parseredirs(cmd, ps, es);
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  if(peek(ps, es, "("))
    return parseblock(ps, es);

  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|)&;")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a')
      panic("syntax");
    cmd->argv[argc] = q;
    cmd->eargv[argc] = eq;
    argc++;
    if(argc >= MAXARGS)
      panic("too many args");
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  cmd->eargv[argc] = 0;
  return ret;
}

// NUL-terminate all the counted strings.
struct cmd*
nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    return 0;

  switch(cmd->type){
  case EXEC:
    ecmd = (struct execcmd*)cmd;
    for(i=0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    nulterminate(lcmd->left);
    nulterminate(lcmd->right);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    nulterminate(bcmd->cmd);
    break;
  }
  return cmd;
}

// Add command to history (circular buffer)
void
add_to_history(char *cmd)
{
  // Skip empty commands and duplicate of last command
  if (cmd[0] == '\n' || cmd[0] == 0) return;
  if (history_count > 0 && strcmp(history[(history_start + history_count - 1) % MAX_HISTORY].command, cmd) == 0) {
    return;
  }
  
  int index = (history_start + history_count) % MAX_HISTORY;
  strcpy(history[index].command, cmd);
  history[index].length = strlen(cmd);
  
  if (history_count < MAX_HISTORY) {
    history_count++;
  } else {
    history_start = (history_start + 1) % MAX_HISTORY;
  }
  
  history_index = -1; // Reset history navigation
}

// Display command history
void
display_history(void)
{
  printf("\n=== Command History ===\n");
  if (history_count == 0) {
    printf("No commands in history.\n");
    return;
  }
  
  for (int i = 0; i < history_count; i++) {
    int index = (history_start + i) % MAX_HISTORY;
    
    // Simple number display
    printf("Command ");
    if (i + 1 == 1) printf("1");
    else if (i + 1 == 2) printf("2");
    else if (i + 1 == 3) printf("3");
    else if (i + 1 == 4) printf("4");
    else if (i + 1 == 5) printf("5");
    else if (i + 1 == 6) printf("6");
    else if (i + 1 == 7) printf("7");
    else if (i + 1 == 8) printf("8");
    else if (i + 1 == 9) printf("9");
    else printf("10+");
    printf(": %s", history[index].command);
  }
  printf("\nUse !1, !2, !3, etc. to execute commands\n\n");
}

// Get history command for navigation (simplified - just return previous/next)
char*
get_history_command(int direction)
{
  if (history_count == 0) return 0;
  
  if (direction > 0) { // Up arrow - go to previous command
    if (history_index == -1) {
      history_index = history_count - 1;
    } else if (history_index > 0) {
      history_index--;
    }
  } else { // Down arrow - go to next command
    if (history_index >= 0) {
      history_index++;
      if (history_index >= history_count) {
        history_index = -1;
        return 0; // No more history
      }
    }
  }
  
  if (history_index >= 0) {
    int index = (history_start + history_index) % MAX_HISTORY;
    return history[index].command;
  }
  
  return 0;
}

// Simple atoi implementation
int
atoi_simple(char *str)
{
  int result = 0;
  int sign = 1;
  
  // Skip whitespace
  while (*str == ' ' || *str == '\t' || *str == '\n') {
    str++;
  }
  
  // Handle sign
  if (*str == '-') {
    sign = -1;
    str++;
  } else if (*str == '+') {
    str++;
  }
  
  // Convert digits
  while (*str >= '0' && *str <= '9') {
    result = result * 10 + (*str - '0');
    str++;
  }
  
  return sign * result;
}

// Simple strncmp implementation
int
strncmp_simple(char *s1, char *s2, int n)
{
  for (int i = 0; i < n; i++) {
    if (s1[i] != s2[i]) {
      return s1[i] - s2[i];
    }
    if (s1[i] == '\0') {
      return 0;
    }
  }
  return 0;
}

// Simple getchar implementation
int
getchar_simple(void)
{
  char c;
  if(read(0, &c, 1) == 1) {
    return (unsigned char)c;
  }
  return -1;
}
