#define _POSIX_C_SOURCE 200809L


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define DEFAULT_WIDTH 40
#define DEFAULT_HEIGHT 20
#define CELL_SIZE 8

static const int NEIGHBOURS[8][2] = {
    {-1, -1}, {0, -1}, {1, -1},
    {-1, 0 },         {1, 0 },
    {-1, 1 }, {0, 1 }, {1, 1 },
};
static struct termios original_termios;

typedef struct {
    size_t width;
    size_t height;
    uint8_t *cells;
    uint8_t *next;

} Life;

static int
LifeInit(Life *life, size_t width, size_t height)
{
    life->width = width;
    life->height = height;

    life->cells = calloc(width * height, sizeof *life->cells);
    life->next = calloc(width *height, sizeof *life->next);

    if (!life->cells || !life->next) {
        free(life->cells);
        free(life->next);
        return -1;
    }
    return 0;
}
static void
LifeDestroy(Life *life)
{
    free(life->cells);
    free(life->next);
}
static int
CountNeighbour(const Life *life, size_t x, size_t y){
    int count = 0; 

    for (size_t i = 0; i < 8 ; i++) {
        int nx = (int)x + NEIGHBOURS[i][0];
        int ny = (int)y + NEIGHBOURS[i][1];

        if (nx < 0 )
            nx = life->width - 1;
        else if (nx >= (int)life->width)
            nx = 0;
        if (ny<0)
            ny = life->height - 1;
        else if (ny >= (int)life->height)
            ny = 0;
        count += life->cells[ny * life->width + nx];
        // if (nx < 0 || ny < 0 ||
        //         nx >= (int)life->width || ny >= (int)life->height)
        //     continue;
        // count += life->cells[ny * life->width + nx];
    }
    return count;
}
static int
LifeStep(Life *life)
{
    int alive = 0;

   for (size_t y = 0; y < life->height; y++){
        for (size_t x = 0; x < life->width; x++){
            size_t i = y * life->width + x;
            int neighbours = CountNeighbour(life, x, y);

           life->next[i] = 
               neighbours == 3 || 
               (life->cells[i] && neighbours == 2);
           alive |= life->next[i];
        
        } 
   } 
   uint8_t *tmp = life->cells;
   life->cells = life->next;
   life->next = tmp;

   return alive;

}
static void
ToggleCell(Life *life, size_t x, size_t y)
{
    if (x >= life->width || y >= life->height)
        return;
    size_t i = y * life->width + x;
    life->cells[i] = !life->cells[i];
}
static void
DrawLife(const Life *life, size_t cursor_x, size_t cursor_y)
{
    printf("\033[H");
    for (size_t y = 0; y < life->height; y++) {
        for (size_t x = 0; x < life->width; x++) {
                size_t i = y * life->width + x;
                
                if (x == cursor_x && y == cursor_y)
                    putchar('@');
                else
                    putchar(life->cells[i] ? '*' : '.');
        }
        putchar('\n');
    }
    fflush(stdout);
}
static void
HandleInput(Life *life, size_t *cursor_x, size_t *cursor_y, 
        int *paused, int *running)
{
    int c = getchar();

    switch (c) {
        case 'q':
            *running  = 0;
            break;
        case ' ':
            *paused = !*paused;
            break;

        case 'h':
            if (*cursor_x > 0)
                (*cursor_x)--;
            break;

        case 'l':
            if (*cursor_x + 1 < life->width)
                (*cursor_x)++;
            break;
        case 'k':
            if (*cursor_y > 0)
                (*cursor_y)--;
            break;
        case 'j':
            if (*cursor_y + 1 < life->height)
                (*cursor_y)++;
            break;

        case '\n':
        case 'a':
            ToggleCell(life, *cursor_x, *cursor_y);
            break;
    }
}

static void
TerminalInit(void)
{
    struct termios raw;

    tcgetattr(STDIN_FILENO, &original_termios);
    raw = original_termios;

    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}
static void
TerminalRestore(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}



int
main(int argc, char *argv[])
{
    
    size_t width = DEFAULT_WIDTH;
    size_t height = DEFAULT_HEIGHT;
    
    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 100000000
    };

    if (argc == 3) {
        width = strtoul(argv[1],NULL,10);
        height = strtoul(argv[2],NULL,10);
    } else if (argc != 1) {
        fprintf(stderr, "usage: %s [width height]\n", argv[0]);
        return 1;

    }
    Life life;
    if (LifeInit(&life, width, height) < 0)
        return 1;


    size_t cursor_x = life.width / 2;
    size_t cursor_y = life.height / 2;
    int paused = 1;
    int running = 1;

    TerminalInit();
    printf("\033[2J");

    while (running) {
        HandleInput(&life, &cursor_x, &cursor_y, &paused, &running);
        if (!paused)
            paused = !LifeStep(&life);
        DrawLife(&life, cursor_x, cursor_y);
        nanosleep(&delay, NULL);

    }
    TerminalRestore();
    /* */
    LifeDestroy(&life);
    return 0;

}
