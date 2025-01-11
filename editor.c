#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "editor.h"

// set the 3 leftmost bits of the key to 0
#define CTRL_KEY(k) (k & 0x1f)
#define NORMAL_MODE 0
#define INSERT_MODE 1

typedef struct
{
    char *content;
    int size;
} line;

// global struct to store editor states
typedef struct
{
    int mode;   // 0 = normal, 1 = insert
    int rows;   // height
    int cols;   // width
    int cx, cy; // cursor positions

    line *lines; // array of lines of text
    int numLines;

    struct termios orig_termios;
} editor;

static editor Editor;
FILE *fp;

void editorRun(void)
{
    fp = fopen("output.txt", "a");
    if (fp == NULL)
    {
        fatal("Error opening file");
    }
    /* const char *text = "Write this to the file"; */
    /* fprintf(fp, "Some text: %s\n", text); */

    // todo: under what circumstances would this function return false?
    if (!isatty((STDIN_FILENO)))
    {
        /* fprintf(stderr, "fatal error: not a terminal\n"); */
        fatal("not a terminal");
    }

    /* printf("test"); */

    editorInit();
    editorRender();

    fclose(fp);
}

void editorInit(void)
{
    // enable raw mode
    //
    // terminals have two modes: cooked or raw. By default, they are cooked. In
    // cooked mode, all characters are echoed to the screen and are buffered
    // internally until the user presses Enter. In raw mode, the characters are
    // not processed by the OS but are available immediately to the program as
    // the user types.
    if (tcgetattr(STDIN_FILENO, &Editor.orig_termios) < 0)
        fatal("failed to get termios settings");

    if (atexit(onExit) != 0)
        fatal("atexit"); // disable raw mode on exit

    // https://www.man7.org/linux/man-pages/man3/termios.3.html
    struct termios t = Editor.orig_termios;
    cfmakeraw(&t);
    t.c_cc[VMIN] = 0;  // minimum number of bytes for read() to return
    t.c_cc[VTIME] = 1; // timeout for read(), wait 1/10 of a second for input
                       // and then return

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) < 0)
        fatal("failed to enable raw mode");

    // initialize Editor
    /* Editor.mode = NORMAL_MODE; */
    Editor.mode = INSERT_MODE;
    Editor.cx = 0;
    Editor.cy = 0;
    Editor.lines = NULL;
    Editor.numLines = 0;
    getWindowSize(&Editor.rows, &Editor.cols);
}

void editorInsertLine(int at)
{
    if (at < 0 || at > Editor.numLines)
        return;

    Editor.lines = realloc(Editor.lines, sizeof(line) * (Editor.numLines + 1));
    memmove(&Editor.lines[at + 1], &Editor.lines[at],
            sizeof(line) * (Editor.numLines - at));

    line *l = at == 0 ? NULL : &Editor.lines[at - 1];
    line *newLine = &Editor.lines[at];

    if (l != NULL && Editor.cx < l->size)
    {
        newLine->size = l->size - Editor.cx;
        newLine->content = malloc(newLine->size + 1);
        memcpy(newLine->content, l->content + Editor.cx, newLine->size);
        newLine->content[newLine->size] = '\0';

        l->size = Editor.cx;
        l->content[l->size] = '\0';
    }
    else
    {
        newLine->size = 0;
        newLine->content = malloc(1);
        newLine->content[newLine->size] = '\0';
    }

    Editor.cx = 0;
    Editor.cy++;
    Editor.numLines++;
}
/* if ((Editor.lines = */
/*          realloc(Editor.lines, ++Editor.numLines * sizeof(line))) == NULL) */
/*     fatal("new lines (enter key) realloc"); */

/* int row = Editor.cy; */

/* if (Editor.numLines > 1 && row < Editor.numLines - 1) */
/* { */
/*     memmove(&Editor.lines[row + 2], &Editor.lines[row + 1], */
/*             sizeof(line) * (Editor.numLines - (row + 1))); */
/* } */

/* int lineSize = Editor.lines[row].size; */
/* if (Editor.cx < lineSize) */
/* { */
/*     // copy the content to right of cursor to the newly created line */
/*     line *l = &Editor.lines[++Editor.cy]; */
/*     l->size = lineSize - Editor.cx; */
/*     l->content = malloc(l->size + 1); */
/*     memcpy(l->content, Editor.lines[row].content + Editor.cx, l->size); */
/*     l->content[l->size] = '\0'; */

/*     Editor.lines[row].size = Editor.cx; */
/* } */
/* else */
/* { */
/*     line *l = &Editor.lines[++Editor.cy]; */
/*     l->size = 0; */
/*     l->content = malloc(1); */
/*     l->content[0] = '\0'; */
/* } */

/* Editor.cx = 0; */
/* return; */
/* } */

void editorInsert(char c)
{
    if (Editor.mode != INSERT_MODE)
        return;

    if (Editor.numLines == 0)
    {
        editorInsertLine(Editor.cy);
        Editor.cy--;
    }

    if (Editor.cy >= Editor.numLines) {
        editorInsertLine(Editor.cy);
        Editor.cy--;
    }

    line *l = &Editor.lines[Editor.cy];

    int pos = Editor.cx;
    if ((l->content = realloc(l->content, l->size + 2)) == NULL)
        fatal("can't allocate memory for line");

    memmove(&l->content[pos + 1], &l->content[pos],
            l->size - pos + 1); // add 1 to account for null char

    l->content[pos] = c;
    l->size++;

    Editor.cx++;
}

void editorDelete()
{
    if (Editor.numLines == 0)
        return;

    line *l = &Editor.lines[Editor.cy];

    // delete characters
    if (l->size > 0 && Editor.cx > 0)
    {
        memmove(&l->content[Editor.cx - 1], &l->content[Editor.cx],
                l->size - Editor.cx + 1);
        l->size--;
        Editor.cx--;
        return;
    }

    // delete row if cursor is at the beginning of the line and move the content
    // if necessary
    if (Editor.cx == 0 && Editor.cy > 0)
    {
        // delete line and append the content of this line to the one above it
        line *lineAbove = &Editor.lines[Editor.cy - 1];
        Editor.cx = lineAbove->size;

        lineAbove->content =
            realloc(lineAbove->content, lineAbove->size + l->size + 1);
        if (lineAbove->content == NULL)
        {
            fatal("realloc for line above during deletion");
        }

        memcpy(&lineAbove->content[lineAbove->size], l->content, l->size);
        lineAbove->size += l->size;
        lineAbove->content[lineAbove->size] = '\0';

        l->size = 0;
        free(l->content);

        int row = Editor.cy;
        if (row < Editor.numLines - 1)
        {
            memmove(&Editor.lines[row], &Editor.lines[row + 1],
                    sizeof(line) * (Editor.numLines - (row + 1)));
        }
        // TODO: this causes some weird error for some reason???
        /* char *buf; */
        /* if ((buf = malloc(l->size)) == NULL) */
        /* { */
        /*     fatal("malloc for buffer deletion"); */
        /* } */

        /* strcpy(buf, l->content); */
        /* strcat(lineAbove->content, buf); */

        Editor.numLines--;
        Editor.cy--;
    }
}

void editorClearScreen(void)
{
    write(STDOUT_FILENO, "\x1b[2J", 4); // Erase In Display (clear screen)
    write(STDOUT_FILENO, "\x1b[H",
          3); // CUP (set cursor position, default arguments are 1 and 1)
    write(STDOUT_FILENO, "\x1b[2 q", 5);
}

char editorReadKey()
{
    char c;
    int result;
    // keep on looping until we receive a character or an error happens
    while ((result = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if (result == -1 && errno != EAGAIN)
            perror("read error");
    }

    // arrow keys mappings
    if (c == '\x1b')
    {
        char buf[2];

        if (read(STDIN_FILENO, &buf[0], 1) == -1)
            fatal("read");
        if (read(STDIN_FILENO, &buf[1], 1) == -1)
            fatal("read");

        if (buf[0] == '[')
        {
            switch (buf[1])
            {
            case 'A':
                return 'k';
            case 'B':
                return 'j';
            case 'C':
                return 'l';
            case 'D':
                return 'h';
            }
        }
    }

    return c;
}

void editorHandleKeypress(char c)
{
    int lineSize;
    switch (c)
    {
    // quit
    case 'q':
        editorClearScreen();
        exit(0);
        break;

    // commands
    case ':':
        break;

    case '\r':
        if (Editor.numLines == 0)
        {
            editorInsertLine(Editor.cy);
        }
        else
        {
            editorInsertLine(Editor.cy + 1);
        }
        break;

    // delete
    case 127:
        editorDelete();
        break;

    // movements
    case 'j':
        if (Editor.cy == Editor.rows - 1 || Editor.cy >= Editor.numLines - 1)
            break;

        lineSize = Editor.lines[Editor.cy + 1].size;
        if (lineSize < Editor.cx)
            Editor.cx = lineSize;

        Editor.cy++;
        break;
    case 'k':
        if (Editor.cy == 0)
            break;

        lineSize = Editor.lines[Editor.cy - 1].size;
        if (lineSize < Editor.cx)
            Editor.cx = lineSize;

        Editor.cy--;
        break;
    case 'h':
        if (Editor.cx == 0)
            break;
        Editor.cx--;
        break;
    case 'l':
        if (Editor.cx == Editor.cols - 1)
            break;

        if (Editor.numLines == 0)
            break;

        if (Editor.mode == NORMAL_MODE &&
            Editor.cx >= Editor.lines[Editor.cy].size - 1)
            break;

        if (Editor.mode == INSERT_MODE &&
            Editor.cx >= Editor.lines[Editor.cy].size)
            break;

        Editor.cx++;
        break;
    default:
        editorInsert(c);
        break;
    }
}

void editorUpdate()
{
    write(STDOUT_FILENO, "\x1b[?25l", 6);

    write(STDOUT_FILENO, "\x1b[H", 3);

    int row;
    for (row = 0; row < Editor.rows; row++)
    {
        if (row < Editor.numLines)
        {
            write(STDOUT_FILENO, Editor.lines[row].content,
                  Editor.lines[row].size);

            write(STDOUT_FILENO, "\x1b[K", 3);
            write(STDOUT_FILENO, "\r\n", 2);
        }
        else
        {
            write(STDOUT_FILENO, "\x1b[2K", 4); // clear entire line
        }
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", Editor.cy + 1, Editor.cx + 1);
    write(STDOUT_FILENO, buf, strlen(buf));

    write(STDOUT_FILENO, "\x1b[?25h", 6);

    if (Editor.mode == INSERT_MODE)
    {
        // https://gitlab.com/gnachman/iterm2/-/issues/2248
        write(STDOUT_FILENO, "\x1b[6 q", 5);
    }
}

int editorRender(void)
{
    editorClearScreen();

    // the main event loop
    while (1)
    {
        editorUpdate();
        editorHandleKeypress(editorReadKey());
    }

    return 0;
}

void appendString(char *original, char *toAppend)
{
}

void getWindowSize(int *rows, int *cols)
{
    struct winsize s;

    // https://man7.org/linux/man-pages/man2/ioctl.2.html
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &s) == -1)
        fatal("window size");

    *rows = s.ws_row;
    *cols = s.ws_col;
}

void fatal(const char *msg)
{
    /* fprintf(stderr, "fatal error: %s\n", msg); */
    perror(msg);
    exit(1);
}

void onExit(void)
{
    // disable raw mode
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &Editor.orig_termios) < 0)
    {
        fatal("failed to diable raw mode");
    }
}
