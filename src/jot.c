/*** INCLUDES ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/*** DEFINES ***/
#define CTRL_KEY(k) ((k) & 0x1f)
/*** DATA ***/
struct editorConfig{
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

struct editorConfig E;

/*** TERMINAL ***/

/**
 * @brief Prints Error and Exits Program
 * 
 * @param s Message to append to error, usually a location or process name
 */
void die(const char *s){
    // 2=entire screen, J=clear
    write(STDOUT_FILENO, "\x1b[2J", 4);
    //Move cursor to start of line
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

/**
 * @brief Reset terminal to original state on fucntion exit
 * 
 */
void disableRawMode(){
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) die("tcgetattr");
}

/**
 * @brief Enable raw mode in termal to read input as it is typed
 * 
 */
void enableRawMode(){
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = E.orig_termios;
    //Turn off input flags. Break Condition | Ctrl-M | Parity Checking | Strip 8th bit of each byte |Ctrl-S Ctrl-Q
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    //Turn off all output processing;
    raw.c_oflag &= ~(OPOST);
    //Sets character size to 8 bits per byte
    raw.c_cflag |= (CS8);
    //Turn off local flags. Echoing  | Canonical Mode | Ctrl-V | Ctrl-C Ctrl-Z
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    //Control Characters
    //Min bytes needed before read() returns
    raw.c_cc[VMIN] = 0;
    //Min time to wait before read returns 
    raw.c_cc[VTIME] = 1;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}
/**
 * @brief wait for keypress and return it
 * 
 * @return char input keypress
 */
char editorReadKey(){
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

/**
 * @brief Get the Window Size (col, row)
 * 
 * @param rows pointer to row int
 * @param cols pointer to col int
 * @return int 0:Success, -1: Failure
 */
int getWindowSize(int *rows, int *cols) {
    struct winsize ws;
    if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        editorReadKey();
        return -1;
    }else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/*** OUTPUT ***/
/**
 * @brief Draws each row of the buffer of text being edited
 * 
 */
void editorDrawRows(){
    int y;
    for (y=0; y<E.screenrows; y++){
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

/**
 * @brief Writes 4 bytes to terminal to clear the display
 * \x1b[ starts all escape sequences
 */
void editorRefreshScreen(){
    // 2=entire screen, J=clear
    write(STDOUT_FILENO, "\x1b[2J", 4);
    //Move cursor to start of line
    write(STDOUT_FILENO, "\x1b[H", 3);
    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** INPUT ***/
/**
 * @brief waits for keypress and handles it
 * 
 */
void editorProcessKeypress(){
    char c = editorReadKey();
    switch (c) {
        case CTRL_KEY('q'):
         // 2=entire screen, J=clear
        write(STDOUT_FILENO, "\x1b[2J", 4);
        //Move cursor to start of line
        write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}

/*** INIT ***/

/**
 * @brief Initialize all Fields in E
 * 
 */
void initEditor(){
    if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main() {
    enableRawMode();
    initEditor();
    while (1){
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}
