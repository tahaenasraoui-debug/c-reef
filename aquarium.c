#ifndef _WIN32
#define _POSIX_C_SOURCE 200112L
#endif

#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#endif

/* =========================================================================
 * CONSTANTS & ANSI PALETTE
 * ========================================================================= */
#define MAX_ENTITIES 80
#define MAX_BUBBLES  120
#define MAX_KELP     16
#define FPS          30
#ifndef _WIN32
#define FRAME_NS     (1000000000L / FPS)
#endif
#define MIN_COLS     30
#define MIN_ROWS     12

enum ColorID {
    COL_RESET = 0,
    COL_WATER_BUBBLE, /* \x1b[96m - Bright Cyan */
    COL_MINNOW,       /* \x1b[94m - Electric Blue */
    COL_GUPPY,        /* \x1b[95m - Neon Magenta */
    COL_PUFFER,       /* \x1b[93m - Bright Yellow */
    COL_ANGEL,        /* \x1b[33m - Coral Gold */
    COL_CLOWN_BODY,   /* \x1b[38;5;208m - Vivid Orange */
    COL_CLOWN_STRIPE, /* \x1b[97m - Bright White */
    COL_EEL,          /* \x1b[32m - Olive Green */
    COL_SWORDFISH,    /* \x1b[34m - Deep Ocean Blue */
    COL_FLOUNDER,     /* \x1b[90m - Sandy Mottled Gray */
    COL_TUNA,         /* \x1b[37m - Silver Gray */
    COL_SHARK,        /* \x1b[90m - Dark Steel / Slate */
    COL_SQUID,        /* \x1b[35m - Violet / Purple */
    COL_CRAB,         /* \x1b[91m - Crustacean Crimson */
    COL_KELP,         /* \x1b[92m - Vivid Green */
    COL_SAND,         /* \x1b[33m - Dark Khaki */
    COL_ROCK,         /* \x1b[90m - Dim Gray */
    COL_COUNT
};

static const char *ANSI_TABLE[] = {
    "\x1b[0m",
    "\x1b[96m",
    "\x1b[94m",
    "\x1b[95m",
    "\x1b[93m",
    "\x1b[33m",
    "\x1b[38;5;208m",
    "\x1b[97m",
    "\x1b[32m",
    "\x1b[34m",
    "\x1b[90m",
    "\x1b[37m",
    "\x1b[90m",
    "\x1b[35m",
    "\x1b[91m",
    "\x1b[92m",
    "\x1b[33m",
    "\x1b[90m"
};

/* =========================================================================
 * SPRITE DEFINITIONS (GEOMETRY AND SYMMETRY PRESERVED)
 * ========================================================================= */
static const char *SPR_MINNOW_R[] = { "><>" };
static const char *SPR_MINNOW_L[] = { "<><" };

static const char *SPR_GUPPY_R[] = {
    "  ,-.==<",
    "><( \xc2\xb0)"
};
static const char *SPR_GUPPY_L[] = {
    "  >==.-.",
    "     (\xc2\xb0 )><"
};

static const char *SPR_PUFFER_R[] = {
    "  .---.",
    ">( 'o' )*",
    "  '---'"
};
static const char *SPR_PUFFER_L[] = {
    "  .---.",
    "*( 'o' )<",
    "  '---'"
};

static const char *SPR_ANGEL_R[] = {
    "  /|",
    ">/ |__ ",
    ">\\ |  ~",
    "  \\|"
};
static const char *SPR_ANGEL_L[] = {
    "   |\\",
    " __| \\<",
    "~  | />",
    "   |/"
};

static const char *SPR_CLOWN_R[] = {
    "    _.-\"\"\\",
    "  .' |   |`>==<",
    " < (o| | | )==<",
    "  `'-|___|/"
};
static const char *SPR_CLOWN_L[] = {
    "     /\"\"-._",
    " >==<|   | '.",
    " >==(| | |o) >",
    "     \\|___|-'`"
};

static const char *SPR_EEL_R[] = {
    " _.~\"~._.~\"~._.--(\xc2\xb0 >",
    "(_.~\"~._.~\"~._.~\"~'"
};
static const char *SPR_EEL_L[] = {
    " < \xc2\xb0)--._.~\"~._.~\"~._",
    "  '~\"~._.~\"~._.~\"~._)"
};

static const char *SPR_SWORD_R[] = {
    "    /|",
    " --' |_____",
    "<====|  o  \\====>",
    "      \\____/"
};
static const char *SPR_SWORD_L[] = {
    "          |\\",
    "     _____| '--",
    "<====/  o  |====>",
    "     \\____/"
};

static const char *SPR_FLOUNDER_R[] = {
    " _.-\"\"\"\"-._",
    "< (o)(o)   >==<",
    " `'-....-'`"
};
static const char *SPR_FLOUNDER_L[] = {
    "      _.-\"\"\"\"-._",
    "  >==<   (o)(o) )>",
    "      `'-....-'`"
};

static const char *SPR_TUNA_R[] = {
    "     |\\",
    "  __/  \\_________",
    " /  o            \\====>",
    " \\________   ____/",
    "          \\_/"
};
static const char *SPR_TUNA_L[] = {
    "            /|",
    "  _________/  \\__",
    "<====\\            o  \\",
    "      \\____   ________/",
    "           \\_/"
};

static const char *SPR_SHARK_R[] = {
    "        /|",
    "       / |",
    " _____/  |_____",
    "/  o           \\______===>",
    "|   \\____   ____/",
    " \\_______|_/"
};
static const char *SPR_SHARK_L[] = {
    "         |\\",
    "         | \\",
    "    _____|  \\_____",
    "<===/           o  \\",
    "    \\____   ____/   |",
    "         \\_|_______/"
};

static const char *SPR_SQUID[] = {
    "      / \\",
    "     /   \\",
    "    /     \\",
    "   |  o o  |",
    "   / / | \\ \\",
    "  / / / \\ \\ \\",
    " ( ( (   ) ) )"
};

static const char *SPR_CRAB_A[] = {
    "  (\\       /)",
    "   \\.-'\"\"'-./",
    "  < ( o  o ) >",
    "   /'-.__.-'\\",
    "  / /  ||  \\ \\"
};
static const char *SPR_CRAB_B[] = {
    "  ()       ()",
    "   \\.-'\"\"'-./",
    "  <(  o  o  )>",
    "   /'-.__.-'\\",
    "  \\ \\  ||  / /"
};

/* =========================================================================
 * DATA STRUCTURES
 * ========================================================================= */
typedef enum {
    SPECIES_MINNOW = 0,
    SPECIES_GUPPY,
    SPECIES_PUFFER,
    SPECIES_ANGEL,
    SPECIES_CLOWN,
    SPECIES_EEL,
    SPECIES_SWORD,
    SPECIES_FLOUNDER,
    SPECIES_TUNA,
    SPECIES_SHARK,
    SPECIES_SQUID,
    SPECIES_CRAB
} SpeciesType;

typedef struct {
    int active;
    SpeciesType type;
    float x, y;
    float vx, vy;
    int dir;
    float phase;
    int anim_timer;
    int anim_frame;
} Entity;

typedef struct {
    int active;
    float x, y;
    float vy;
    char ch;
} Bubble;

typedef struct {
    int x;
    int height;
    float phase;
} Kelp;

/* =========================================================================
 * GLOBAL STATE & BUFFERS
 * ========================================================================= */
static volatile sig_atomic_t g_running = 1;
static volatile sig_atomic_t g_resized = 0;

static int g_width = 80;
static int g_height = 24;

static char **g_char_buf = NULL;
static unsigned char **g_col_buf = NULL;
static char *g_sand_row1 = NULL;
static char *g_sand_row2 = NULL;
static unsigned char *g_sand_col1 = NULL;
static unsigned char *g_sand_col2 = NULL;
static char *g_out_buf = NULL;
static size_t g_out_buf_cap = 0;

static Entity g_entities[MAX_ENTITIES];
static Bubble g_bubbles[MAX_BUBBLES];
static Kelp   g_kelps[MAX_KELP];
static int    g_num_kelps = 0;
static unsigned long g_tick = 0;
static int    g_sound = 0;        /* --sound */
static int    g_use_color = 1;    /* cleared by NO_COLOR */
static int    g_buffers_ok = 0;   /* 0 while the terminal is too small */
#ifdef _WIN32
static DWORD  g_saved_out_mode = 0;
static int    g_out_mode_saved = 0;
static UINT   g_saved_cp = 0;
static int    g_cp_saved = 0;
static int    g_last_term_w = 0;
static int    g_last_term_h = 0;
#else
static pid_t  g_audio_pid = -1;
static int    g_raw_enabled = 0;
static struct termios g_saved_termios;
#endif

/* =========================================================================
 * UTILITIES & PLATFORM TERMINAL / SIGNAL HANDLING
 * ========================================================================= */
#ifdef _WIN32
static BOOL WINAPI handle_ctrl(DWORD ctrl_type) {
    switch (ctrl_type) {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
            g_running = 0;
            return TRUE;
        default:
            return FALSE;
    }
}

static void install_signal_handlers(void) {
    SetConsoleCtrlHandler(handle_ctrl, TRUE);
}

static int is_stdout_tty(void) {
    return _isatty(_fileno(stdout)) != 0;
}

static void get_terminal_size(int *out_w, int *out_h) {
    int w = 80;
    int h = 24; /* fallback if redirected or unavailable */
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (hOut != INVALID_HANDLE_VALUE && hOut != NULL &&
        GetConsoleScreenBufferInfo(hOut, &csbi)) {
        int cw = (int)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
        int ch = (int)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
        if (cw > 0 && ch > 0) {
            w = cw;
            h = ch;
        }
    }
    *out_w = w;
    *out_h = h;
}

static void write_all(const char *buf, size_t len) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    size_t off = 0;
    if (hOut != INVALID_HANDLE_VALUE && hOut != NULL) {
        while (off < len) {
            DWORD chunk = (len - off > 65536U) ? (DWORD)65536U : (DWORD)(len - off);
            DWORD written = 0;
            if (!WriteFile(hOut, buf + off, chunk, &written, NULL) || written == 0) {
                break;
            }
            off += (size_t)written;
        }
        return;
    }
    int fd = _fileno(stdout);
    while (off < len) {
        unsigned int chunk = (len - off > 65536U) ? 65536U : (unsigned int)(len - off);
        int n = _write(fd, buf + off, chunk);
        if (n <= 0) break;
        off += (size_t)n;
    }
}

static void setup_terminal(void) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE && hOut != NULL) {
        if (GetConsoleMode(hOut, &g_saved_out_mode)) {
            g_out_mode_saved = 1;
            SetConsoleMode(hOut, g_saved_out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
    UINT cp = GetConsoleOutputCP();
    if (cp != 0) {
        g_saved_cp = cp;
        g_cp_saved = 1;
    }
    SetConsoleOutputCP(CP_UTF8);
}

static void restore_terminal_input(void) {
    /* Nothing to restore before writing exit_seq on Windows. */
}

static void restore_terminal_output(void) {
    if (g_out_mode_saved) {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE && hOut != NULL) {
            SetConsoleMode(hOut, g_saved_out_mode);
        }
        g_out_mode_saved = 0;
    }
    if (g_cp_saved) {
        SetConsoleOutputCP(g_saved_cp);
        g_cp_saved = 0;
    }
}

static void poll_input(void) {
    while (_kbhit()) {
        int ch = _getch();
        if (ch == 0 || ch == 0xE0) {
            if (_kbhit()) (void)_getch(); /* discard extended key scan code */
            continue;
        }
        if (ch == 'q' || ch == 'Q' || ch == 3) {
            g_running = 0;
            break;
        }
    }
}

static void poll_resize(void) {
    int cur_w = 0;
    int cur_h = 0;
    get_terminal_size(&cur_w, &cur_h);
    if (cur_w != g_last_term_w || cur_h != g_last_term_h) {
        g_last_term_w = cur_w;
        g_last_term_h = cur_h;
        g_resized = 1;
    }
}

typedef struct {
    LARGE_INTEGER freq;
    LARGE_INTEGER next;
} FrameTimer;

static void frame_timer_init(FrameTimer *ft) {
    QueryPerformanceFrequency(&ft->freq);
    QueryPerformanceCounter(&ft->next);
}

static void frame_timer_wait(FrameTimer *ft) {
    LARGE_INTEGER now;
    LONGLONG step = (ft->freq.QuadPart > 0) ? (ft->freq.QuadPart / FPS) : 1;
    if (step < 1) step = 1;

    ft->next.QuadPart += step;
    QueryPerformanceCounter(&now);
    if (now.QuadPart >= ft->next.QuadPart) {
        /* Fell behind: resync instead of bursting. */
        ft->next = now;
        return;
    }

    if (ft->freq.QuadPart > 0) {
        LONGLONG remaining = ft->next.QuadPart - now.QuadPart;
        DWORD ms = (DWORD)((remaining * 1000LL) / ft->freq.QuadPart);
        if (ms > 0) {
            Sleep(ms);
        }
    }
}
#else
static void handle_sig(int sig) {
    (void)sig;
    g_running = 0;
}

static void handle_winch(int sig) {
    (void)sig;
    g_resized = 1;
}

static void install_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sig;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    struct sigaction old_hup;
    if (sigaction(SIGHUP, NULL, &old_hup) == 0 && old_hup.sa_handler != SIG_IGN) {
        sigaction(SIGHUP, &sa, NULL);
    }

    struct sigaction sa_winch;
    memset(&sa_winch, 0, sizeof(sa_winch));
    sa_winch.sa_handler = handle_winch;
    sigaction(SIGWINCH, &sa_winch, NULL);
}

static int is_stdout_tty(void) {
    return isatty(STDOUT_FILENO) != 0;
}

static void get_terminal_size(int *out_w, int *out_h) {
    int w = 80;
    int h = 24; /* fallback if the size cannot be queried */
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        w = ws.ws_col;
        h = ws.ws_row;
    }
    *out_w = w;
    *out_h = h;
}

static void write_all(const char *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(STDOUT_FILENO, buf + off, len - off);
        if (n > 0) {
            off += (size_t)n;
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }
}

static void setup_terminal(void) {
    if (!isatty(STDIN_FILENO)) return;
    int rc;
    while ((rc = tcgetattr(STDIN_FILENO, &g_saved_termios)) < 0 && errno == EINTR) {
    }
    if (rc != 0) return;

    struct termios raw = g_saved_termios;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO); /* keep ISIG so Ctrl+C still works */
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    while ((rc = tcsetattr(STDIN_FILENO, TCSANOW, &raw)) < 0 && errno == EINTR) {
    }
    if (rc == 0) g_raw_enabled = 1;
}

static void restore_terminal_input(void) {
    if (g_raw_enabled) {
        while (tcsetattr(STDIN_FILENO, TCSANOW, &g_saved_termios) < 0 && errno == EINTR) {
        }
        g_raw_enabled = 0;
    }
}

static void restore_terminal_output(void) {
    /* Nothing to restore after writing exit_seq on POSIX. */
}

static void poll_input(void) {
    if (!g_raw_enabled) return;
    char c;
    for (;;) {
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n == 1) {
            if (c == 'q' || c == 'Q') {
                g_running = 0;
                break;
            }
        } else if (n < 0 && errno == EINTR) {
            if (!g_running) break;
            continue;
        } else {
            break;
        }
    }
}

static void poll_resize(void) {
    /* Resize is signalled asynchronously via SIGWINCH on POSIX. */
}

typedef struct {
    struct timespec next;
} FrameTimer;

static void frame_timer_init(FrameTimer *ft) {
    clock_gettime(CLOCK_MONOTONIC, &ft->next);
}

/* Absolute-deadline pacing: sleep time absorbs the work done per frame. */
static void frame_timer_wait(FrameTimer *ft) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    ft->next.tv_nsec += FRAME_NS;
    while (ft->next.tv_nsec >= 1000000000L) {
        ft->next.tv_nsec -= 1000000000L;
        ft->next.tv_sec++;
    }
    /* Fell behind (suspend, slow terminal): resync instead of bursting. */
    if (now.tv_sec > ft->next.tv_sec ||
        (now.tv_sec == ft->next.tv_sec && now.tv_nsec > ft->next.tv_nsec)) {
        ft->next = now;
    }
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ft->next, NULL);
}
#endif

static float randf(float min_v, float max_v) {
    if (max_v <= min_v) return min_v;
    return min_v + ((float)rand() / (float)RAND_MAX) * (max_v - min_v);
}

static void free_buffers(void) {
    if (g_char_buf) {
        for (int r = 0; r < g_height; r++) free(g_char_buf[r]);
        free(g_char_buf);
        g_char_buf = NULL;
    }
    if (g_col_buf) {
        for (int r = 0; r < g_height; r++) free(g_col_buf[r]);
        free(g_col_buf);
        g_col_buf = NULL;
    }
    free(g_sand_row1); g_sand_row1 = NULL;
    free(g_sand_row2); g_sand_row2 = NULL;
    free(g_sand_col1); g_sand_col1 = NULL;
    free(g_sand_col2); g_sand_col2 = NULL;
    free(g_out_buf);   g_out_buf = NULL;
    g_out_buf_cap = 0;
}

static void init_seabed(void) {
    const char sand_chars[] = "._~._~._~.";
    const char rock_chars[] = "Oo@";

    for (int c = 0; c < g_width; c++) {
        if (rand() % 11 == 0) {
            g_sand_row1[c] = rock_chars[rand() % 3];
            g_sand_col1[c] = COL_ROCK;
        } else {
            g_sand_row1[c] = sand_chars[rand() % 10];
            g_sand_col1[c] = COL_SAND;
        }
        if (rand() % 9 == 0) {
            g_sand_row2[c] = rock_chars[rand() % 3];
            g_sand_col2[c] = COL_ROCK;
        } else {
            g_sand_row2[c] = sand_chars[rand() % 10];
            g_sand_col2[c] = COL_SAND;
        }
    }

    g_num_kelps = 5 + (rand() % 2);
    if (g_num_kelps > MAX_KELP) g_num_kelps = MAX_KELP;

    int segment = (g_width > 12) ? (g_width - 8) / g_num_kelps : 4;
    for (int i = 0; i < g_num_kelps; i++) {
        int base_x = 4 + i * segment + (segment > 2 ? (rand() % (segment - 1)) : 0);
        if (base_x >= g_width - 2) base_x = g_width - 3;
        if (base_x < 2) base_x = 2;
        g_kelps[i].x = base_x;
        int max_h = (g_height / 3);
        if (max_h < 5) max_h = 5;
        if (max_h > 14) max_h = 14;
        g_kelps[i].height = 5 + (rand() % (max_h - 3));
        g_kelps[i].phase = randf(0.0f, 6.28318f);
    }
}

/* Returns 1 on success, 0 if smaller than MIN_COLS x MIN_ROWS, -1 on OOM. */
static int allocate_buffers(void) {
    int w = 80, h = 24;
    get_terminal_size(&w, &h);
    if (w < MIN_COLS || h < MIN_ROWS) return 0;

    size_t sw = (size_t)w;
    size_t sh = (size_t)h;
    if (sh > SIZE_MAX / sizeof(char *) ||
        sw > (SIZE_MAX - 16384U) / (sh * 24U)) {
        return -1;
    }

    g_width  = w;
    g_height = h;

    g_char_buf = calloc(sh, sizeof(char *));
    g_col_buf  = calloc(sh, sizeof(unsigned char *));
    if (!g_char_buf || !g_col_buf) {
        free_buffers();
        return -1;
    }
    for (int r = 0; r < g_height; r++) {
        g_char_buf[r] = malloc(sw);
        g_col_buf[r]  = malloc(sw);
        if (!g_char_buf[r] || !g_col_buf[r]) {
            free_buffers();
            return -1;
        }
    }

    g_sand_row1 = malloc(sw);
    g_sand_row2 = malloc(sw);
    g_sand_col1 = malloc(sw);
    g_sand_col2 = malloc(sw);

    size_t cap = sw * sh * 24U + 16384U;
    g_out_buf = malloc(cap);
    if (!g_sand_row1 || !g_sand_row2 || !g_sand_col1 || !g_sand_col2 || !g_out_buf) {
        free_buffers();
        return -1;
    }
    g_out_buf_cap = cap;

    init_seabed();
    return 1;
}

/* =========================================================================
 * SPRITE RENDERING WITH TRANSPARENCY & STRICT CLIPPING
 * ========================================================================= */
#define DEGREE_TOKEN '\x01'

static void draw_sprite_line(int row, int col_start, const char *line, unsigned char default_col, SpeciesType type) {
    if (row < 0 || row >= g_height - 2) return;

    int visual_col = col_start;
    for (int i = 0; line[i] != '\0'; i++) {
        char ch = line[i];
        if ((unsigned char)ch == 0xC2 && (unsigned char)line[i + 1] == 0xB0) {
            ch = DEGREE_TOKEN;
            i++;
        }

        if (ch != ' ') {
            if (visual_col >= 0 && visual_col < g_width) {
                g_char_buf[row][visual_col] = ch;

                if (type == SPECIES_CLOWN && (ch == '|' || ch == '_')) {
                    g_col_buf[row][visual_col] = COL_CLOWN_STRIPE;
                } else if (ch == 'o' || ch == DEGREE_TOKEN) {
                    g_col_buf[row][visual_col] = COL_CLOWN_STRIPE;
                } else {
                    g_col_buf[row][visual_col] = default_col;
                }
            }
        }
        visual_col++;
    }
}

static void draw_entity(const Entity *e) {
    const char **sprite = NULL;
    int lines = 0;
    unsigned char col = COL_RESET;

    switch (e->type) {
        case SPECIES_MINNOW:
            sprite = (e->dir > 0) ? SPR_MINNOW_R : SPR_MINNOW_L;
            lines = 1; col = COL_MINNOW; break;
        case SPECIES_GUPPY:
            sprite = (e->dir > 0) ? SPR_GUPPY_R : SPR_GUPPY_L;
            lines = 2; col = COL_GUPPY; break;
        case SPECIES_PUFFER:
            sprite = (e->dir > 0) ? SPR_PUFFER_R : SPR_PUFFER_L;
            lines = 3; col = COL_PUFFER; break;
        case SPECIES_ANGEL:
            sprite = (e->dir > 0) ? SPR_ANGEL_R : SPR_ANGEL_L;
            lines = 4; col = COL_ANGEL; break;
        case SPECIES_CLOWN:
            sprite = (e->dir > 0) ? SPR_CLOWN_R : SPR_CLOWN_L;
            lines = 4; col = COL_CLOWN_BODY; break;
        case SPECIES_EEL:
            sprite = (e->dir > 0) ? SPR_EEL_R : SPR_EEL_L;
            lines = 2; col = COL_EEL; break;
        case SPECIES_SWORD:
            sprite = (e->dir > 0) ? SPR_SWORD_R : SPR_SWORD_L;
            lines = 4; col = COL_SWORDFISH; break;
        case SPECIES_FLOUNDER:
            sprite = (e->dir > 0) ? SPR_FLOUNDER_R : SPR_FLOUNDER_L;
            lines = 3; col = COL_FLOUNDER; break;
        case SPECIES_TUNA:
            sprite = (e->dir > 0) ? SPR_TUNA_R : SPR_TUNA_L;
            lines = 5; col = COL_TUNA; break;
        case SPECIES_SHARK:
            sprite = (e->dir > 0) ? SPR_SHARK_R : SPR_SHARK_L;
            lines = 6; col = COL_SHARK; break;
        case SPECIES_SQUID:
            sprite = SPR_SQUID;
            lines = 7; col = COL_SQUID; break;
        case SPECIES_CRAB:
            sprite = (e->anim_frame == 0) ? SPR_CRAB_A : SPR_CRAB_B;
            lines = 5; col = COL_CRAB; break;
    }

    int base_y = (int)roundf(e->y);
    int base_x = (int)roundf(e->x);

    for (int r = 0; r < lines; r++) {
        int draw_y = base_y + r;
        int draw_x = base_x;

        if (e->type == SPECIES_EEL) {
            draw_y += (int)roundf(sinf(e->phase + (float)r * 1.4f) * 0.6f);
        }
        draw_sprite_line(draw_y, draw_x, sprite[r], col, e->type);
    }
}

/* =========================================================================
 * ENTITY SPAWNING & PHYSICS
 * ========================================================================= */
static int find_free_slot(void) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!g_entities[i].active) return i;
    }
    return -1;
}

static int count_species(SpeciesType t) {
    int c = 0;
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (g_entities[i].active && g_entities[i].type == t) c++;
    }
    return c;
}

static void spawn_minnow_school(int initial) {
    int pack_size = 3 + (rand() % 3);
    int dir = (rand() % 2 == 0) ? 1 : -1;
    float max_x = (float)(g_width - 15);
    if (max_x < 5.0f) max_x = 5.0f;
    float base_x = initial ? randf(5.0f, max_x)
                           : (dir > 0 ? -8.0f : (float)(g_width + 4));
    float base_y = randf(2.0f, (float)g_height * 0.55f);
    float speed = randf(0.85f, 1.25f) * (float)dir;
    float shared_phase = randf(0.0f, 6.28f);

    for (int i = 0; i < pack_size; i++) {
        int idx = find_free_slot();
        if (idx < 0) break;
        Entity *e = &g_entities[idx];
        memset(e, 0, sizeof(*e));
        e->active = 1;
        e->type   = SPECIES_MINNOW;
        e->dir    = dir;
        e->x      = base_x + (float)(i * 4 * (-dir)) + randf(-1.0f, 1.0f);
        e->y      = base_y + (float)((i % 3) - 1) * 1.2f;
        e->vx     = speed;
        e->vy     = 0.0f;
        e->phase  = shared_phase + (float)i * 0.35f;
    }
}

static float clamp_rand_y(float min_y, float max_y) {
    if (max_y < min_y) max_y = min_y;
    return randf(min_y, max_y);
}

static void spawn_single_species(SpeciesType type, int initial) {
    int idx = find_free_slot();
    if (idx < 0) return;

    Entity *e = &g_entities[idx];
    memset(e, 0, sizeof(*e));
    e->active = 1;
    e->type   = type;
    e->dir    = (rand() % 2 == 0) ? 1 : -1;
    e->phase  = randf(0.0f, 6.28f);

    int sea_bottom = g_height - 2;
    int margin = 28;
    float fdir = (float)e->dir;

    switch (type) {
        case SPECIES_GUPPY:
            e->vx = randf(0.45f, 0.70f) * fdir;
            e->y  = clamp_rand_y(2.0f, (float)(sea_bottom - 8));
            margin = 10;
            break;
        case SPECIES_PUFFER:
            e->vx = randf(0.18f, 0.30f) * fdir;
            e->y  = clamp_rand_y(2.0f, (float)(sea_bottom - 9));
            margin = 12;
            break;
        case SPECIES_ANGEL:
            e->vx = randf(0.28f, 0.45f) * fdir;
            e->y  = clamp_rand_y((float)g_height * 0.2f, (float)g_height * 0.65f);
            margin = 10;
            break;
        case SPECIES_CLOWN:
            e->vx = randf(0.32f, 0.52f) * fdir;
            e->y  = clamp_rand_y((float)g_height * 0.25f, (float)(sea_bottom - 7));
            margin = 18;
            break;
        case SPECIES_EEL: {
            float max_y = (float)(sea_bottom - 4);
            float min_y = (float)g_height * 0.68f;
            if (min_y > max_y) min_y = max_y;
            e->vx = randf(0.35f, 0.55f) * fdir;
            e->y  = clamp_rand_y(min_y, max_y);
            if (e->y < 3.0f) e->y = 3.0f;
            margin = 24;
            break;
        }
        case SPECIES_SWORD:
            e->vx = randf(0.75f, 1.10f) * fdir;
            e->y  = clamp_rand_y(2.0f, (float)g_height * 0.55f);
            margin = 20;
            break;
        case SPECIES_FLOUNDER:
            e->vx = randf(0.22f, 0.38f) * fdir;
            e->y  = (float)(sea_bottom - 4);
            if (e->y < 2.0f) e->y = 2.0f;
            margin = 18;
            break;
        case SPECIES_TUNA:
            e->vx = randf(0.65f, 0.95f) * fdir;
            e->y  = clamp_rand_y(3.0f, (float)g_height * 0.60f);
            margin = 25;
            break;
        case SPECIES_SHARK:
            e->vx = randf(0.22f, 0.34f) * fdir;
            e->y  = clamp_rand_y((float)g_height * 0.30f, (float)g_height * 0.62f);
            margin = 30;
            break;
        case SPECIES_SQUID: {
            float max_y = (float)(sea_bottom - 9);
            float min_y = (float)g_height * 0.35f;
            if (max_y < 2.0f) max_y = 2.0f;
            if (min_y > max_y) min_y = max_y;
            e->vx = randf(0.18f, 0.32f) * fdir;
            e->vy = -0.35f;
            e->y  = clamp_rand_y(min_y, max_y);
            e->anim_timer = 20 + (rand() % 25);
            margin = 16;
            break;
        }
        case SPECIES_CRAB:
            e->vx = randf(0.12f, 0.22f) * fdir;
            e->y  = (float)(sea_bottom - 5);
            if (e->y < 1.0f) e->y = 1.0f;
            e->anim_timer = 15;
            e->anim_frame = 0;
            margin = 16;
            break;
        default:
            break;
    }

    if (initial) {
        float max_x = (float)(g_width - 15);
        if (max_x < 2.0f) max_x = 2.0f;
        e->x = randf(2.0f, max_x);
    } else {
        e->x = (e->dir > 0) ? (float)(-margin) : (float)(g_width + 3);
    }
}

static void populate_aquarium(void) {
    memset(g_entities, 0, sizeof(g_entities));
    memset(g_bubbles, 0, sizeof(g_bubbles));

    spawn_minnow_school(1);
    spawn_minnow_school(1);
    spawn_single_species(SPECIES_GUPPY, 1);
    spawn_single_species(SPECIES_GUPPY, 1);
    spawn_single_species(SPECIES_PUFFER, 1);
    spawn_single_species(SPECIES_ANGEL, 1);
    spawn_single_species(SPECIES_CLOWN, 1);
    spawn_single_species(SPECIES_EEL, 1);
    spawn_single_species(SPECIES_SWORD, 1);
    spawn_single_species(SPECIES_FLOUNDER, 1);
    spawn_single_species(SPECIES_TUNA, 1);
    spawn_single_species(SPECIES_SHARK, 1);
    spawn_single_species(SPECIES_SQUID, 1);
    spawn_single_species(SPECIES_CRAB, 1);
}

static void maintain_population(void) {
    if (count_species(SPECIES_MINNOW) < 4)   spawn_minnow_school(0);
    if (count_species(SPECIES_GUPPY) < 2)    spawn_single_species(SPECIES_GUPPY, 0);
    if (count_species(SPECIES_PUFFER) < 1)   spawn_single_species(SPECIES_PUFFER, 0);
    if (count_species(SPECIES_ANGEL) < 2)    spawn_single_species(SPECIES_ANGEL, 0);
    if (count_species(SPECIES_CLOWN) < 2)    spawn_single_species(SPECIES_CLOWN, 0);
    if (count_species(SPECIES_EEL) < 1)      spawn_single_species(SPECIES_EEL, 0);
    if (count_species(SPECIES_SWORD) < 1)    spawn_single_species(SPECIES_SWORD, 0);
    if (count_species(SPECIES_FLOUNDER) < 1) spawn_single_species(SPECIES_FLOUNDER, 0);
    if (count_species(SPECIES_TUNA) < 1)     spawn_single_species(SPECIES_TUNA, 0);
    if (count_species(SPECIES_SHARK) < 1)    spawn_single_species(SPECIES_SHARK, 0);
    if (count_species(SPECIES_SQUID) < 1)    spawn_single_species(SPECIES_SQUID, 0);
    if (count_species(SPECIES_CRAB) < 1)     spawn_single_species(SPECIES_CRAB, 0);
}

static void spawn_bubble(float x, float y) {
    for (int i = 0; i < MAX_BUBBLES; i++) {
        if (!g_bubbles[i].active) {
            g_bubbles[i].active = 1;
            g_bubbles[i].x = x;
            g_bubbles[i].y = y;
            g_bubbles[i].vy = randf(0.20f, 0.42f);
            g_bubbles[i].ch = (rand() % 2 == 0) ? 'o' : '.';
            return;
        }
    }
}

static void update_simulation(void) {
    int sea_bottom = g_height - 2;

    if (rand() % 3 == 0) {
        if (g_num_kelps > 0 && (rand() % 2 == 0)) {
            int k = rand() % g_num_kelps;
            spawn_bubble((float)g_kelps[k].x, (float)(sea_bottom - g_kelps[k].height));
        } else {
            spawn_bubble(randf(2.0f, (float)(g_width - 3)), (float)(sea_bottom - 1));
        }
    }

    for (int i = 0; i < MAX_BUBBLES; i++) {
        if (!g_bubbles[i].active) continue;
        g_bubbles[i].y -= g_bubbles[i].vy;
        g_bubbles[i].x += randf(-0.22f, 0.22f);
        if (g_bubbles[i].y <= 1.0f || g_bubbles[i].x < 0.0f || g_bubbles[i].x >= (float)g_width) {
            g_bubbles[i].active = 0;
        }
    }

    for (int i = 0; i < MAX_ENTITIES; i++) {
        Entity *e = &g_entities[i];
        if (!e->active) continue;

        e->phase += 0.14f;
        e->x += e->vx;

        switch (e->type) {
            case SPECIES_MINNOW:
                e->y += sinf(e->phase) * 0.08f;
                break;
            case SPECIES_GUPPY:
                e->y += sinf(e->phase * 2.2f) * 0.18f;
                break;
            case SPECIES_PUFFER:
                e->y += sinf(e->phase * 0.7f) * 0.12f;
                break;
            case SPECIES_ANGEL:
            case SPECIES_CLOWN:
                e->y += sinf(e->phase * 0.9f) * 0.07f;
                break;
            case SPECIES_EEL:
                e->y += sinf(e->phase * 1.3f) * 0.11f;
                if (e->y > (float)(sea_bottom - 3)) e->y = (float)(sea_bottom - 3);
                break;
            case SPECIES_FLOUNDER:
                e->y = (float)(sea_bottom - 3);
                break;
            case SPECIES_SQUID:
                e->anim_timer--;
                if (e->anim_timer <= 0) {
                    e->vy = -0.42f;
                    e->vx = (e->dir > 0 ? 1.0f : -1.0f) * randf(0.28f, 0.45f);
                    e->anim_timer = 38 + (rand() % 30);
                } else {
                    e->vy += 0.015f;
                    if (e->vy > 0.24f) e->vy = 0.24f;
                }
                e->y += e->vy;
                if (e->y < 2.0f) { e->y = 2.0f; e->vy = 0.15f; }
                if (e->y > (float)(sea_bottom - 8)) {
                    e->y = (float)(sea_bottom - 8);
                    e->vy = -0.38f;
                }
                break;
            case SPECIES_CRAB:
                e->y = (float)(sea_bottom - 5);
                if (e->y < 1.0f) e->y = 1.0f;
                e->anim_timer--;
                if (e->anim_timer <= 0) {
                    e->anim_frame = !e->anim_frame;
                    e->anim_timer = 12 + (rand() % 14);
                }
                if (e->x < 2.0f && e->dir < 0) { e->dir = 1; e->vx = -e->vx; }
                if (e->x > (float)(g_width - 16) && e->dir > 0) { e->dir = -1; e->vx = -e->vx; }
                break;
            default:
                break;
        }

        if ((e->dir > 0 && e->x > (float)(g_width + 6)) ||
            (e->dir < 0 && e->x < -34.0f)) {
            e->active = 0;
        }
    }

    maintain_population();
}

/* =========================================================================
 * IN-MEMORY DOUBLE-BUFFER RENDER PIPELINE
 * ========================================================================= */
static void out_append(size_t *pos, const char *data, size_t len) {
    if (*pos < g_out_buf_cap && len <= g_out_buf_cap - *pos) {
        memcpy(g_out_buf + *pos, data, len);
        *pos += len;
    }
}

static void out_append_char(size_t *pos, char ch) {
    if (*pos < g_out_buf_cap) {
        g_out_buf[(*pos)++] = ch;
    }
}

static void render_frame(void) {
    size_t row_bytes = (size_t)g_width;
    for (int r = 0; r < g_height; r++) {
        memset(g_char_buf[r], ' ', row_bytes);
        memset(g_col_buf[r], COL_RESET, row_bytes);
    }

    int sea_bottom = g_height - 2;

    if (g_height >= 2) {
        memcpy(g_char_buf[g_height - 2], g_sand_row1, row_bytes);
        memcpy(g_col_buf[g_height - 2],  g_sand_col1, row_bytes);
        memcpy(g_char_buf[g_height - 1], g_sand_row2, row_bytes);
        memcpy(g_col_buf[g_height - 1],  g_sand_col2, row_bytes);
    }

    const char kelp_glyphs[] = "(){}";
    for (int i = 0; i < g_num_kelps; i++) {
        for (int h = 0; h < g_kelps[i].height; h++) {
            int row = sea_bottom - 1 - h;
            if (row < 1 || row >= sea_bottom) continue;
            float sway = sinf((float)g_tick * 0.08f + g_kelps[i].phase + (float)h * 0.45f);
            int col = g_kelps[i].x + (int)roundf(sway * 1.15f);
            if (col >= 0 && col < g_width) {
                char kc = kelp_glyphs[((unsigned long)h + (g_tick / 12UL)) % 4UL];
                g_char_buf[row][col] = kc;
                g_col_buf[row][col]  = COL_KELP;
            }
        }
    }

    for (int i = 0; i < MAX_BUBBLES; i++) {
        if (!g_bubbles[i].active) continue;
        int r = (int)roundf(g_bubbles[i].y);
        int c = (int)roundf(g_bubbles[i].x);
        if (r >= 1 && r < sea_bottom && c >= 0 && c < g_width) {
            g_char_buf[r][c] = g_bubbles[i].ch;
            g_col_buf[r][c]  = COL_WATER_BUBBLE;
        }
    }

    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (g_entities[i].active) {
            draw_entity(&g_entities[i]);
        }
    }

    size_t pos = 0;
    unsigned char cur_col = 255;

    for (int r = 0; r < g_height; r++) {
        char row_seq[16];
        int n = snprintf(row_seq, sizeof(row_seq), "\x1b[%d;1H", r + 1);
        if (n > 0) {
            size_t slen = (n >= (int)sizeof(row_seq)) ? (sizeof(row_seq) - 1) : (size_t)n;
            out_append(&pos, row_seq, slen);
        }

        int safe_cols = (g_width > 1) ? (g_width - 1) : g_width;
        for (int c = 0; c < safe_cols; c++) {
            unsigned char cell_col = g_col_buf[r][c];
            char ch = g_char_buf[r][c];

            if (cell_col != cur_col) {
                if (g_use_color && cell_col < COL_COUNT) {
                    const char *ansi = ANSI_TABLE[cell_col];
                    out_append(&pos, ansi, strlen(ansi));
                }
                cur_col = cell_col;
            }

            if (ch == DEGREE_TOKEN) {
                out_append_char(&pos, '\xc2');
                out_append_char(&pos, '\xb0');
            } else {
                out_append_char(&pos, ch);
            }
        }
    }

    if (g_use_color) {
        const char *rst = ANSI_TABLE[COL_RESET];
        out_append(&pos, rst, strlen(rst));
    }

    write_all(g_out_buf, pos);
}

/* =========================================================================
 * PROCEDURAL AUDIO ENGINE (ALSA / APLAY)
 * ========================================================================= */
#ifndef _WIN32
static void start_halal_ocean_ambience(void) {
    sigset_t block_mask, old_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);
    sigaddset(&block_mask, SIGTERM);
    sigaddset(&block_mask, SIGHUP);
    sigaddset(&block_mask, SIGWINCH);
    sigprocmask(SIG_BLOCK, &block_mask, &old_mask);

    pid_t pid = fork();
    if (pid < 0) {
        sigprocmask(SIG_SETMASK, &old_mask, NULL);
        return;
    }

    if (pid == 0) {
        /* Isolate into its own process group and detach from the terminal
         * so aplay/sh cannot write to the PTY or ignore group termination. */
        setpgid(0, 0);

        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            if (devnull > STDERR_FILENO) close(devnull);
        }

        /* Restore default signal dispositions BEFORE unblocking signals so a
         * parent SIGTERM cannot hit the inherited handler during startup. */
        struct sigaction dfl;
        memset(&dfl, 0, sizeof(dfl));
        dfl.sa_handler = SIG_DFL;
        sigaction(SIGINT,   &dfl, NULL);
        sigaction(SIGTERM,  &dfl, NULL);
        sigaction(SIGHUP,   &dfl, NULL);
        sigaction(SIGWINCH, &dfl, NULL);
        dfl.sa_handler = SIG_IGN;
        sigaction(SIGPIPE,  &dfl, NULL);

        sigprocmask(SIG_SETMASK, &old_mask, NULL);

        FILE *pipe = popen("aplay -q -f S16_LE -r 22050 -c 1 2>/dev/null", "w");
        if (!pipe) _exit(0);

        const int sample_rate = 22050;
        short buffer[1024];
        float brown = 0.0f, lp1 = 0.0f, lp2 = 0.0f, wave_phase = 0.0f;
        float drop_freq = 0.0f, drop_env = 0.0f, drop_phase = 0.0f;

        srand((unsigned int)time(NULL) ^ (unsigned int)getpid());

        while (1) {
            for (int i = 0; i < 1024; i++) {
                float white = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
                brown = (brown + 0.02f * white) / 1.02f;

                wave_phase += 0.12f / (float)sample_rate;
                if (wave_phase > 1.0f) wave_phase -= 1.0f;
                float swell = 0.55f + 0.45f * sinf(wave_phase * 6.283185f);

                lp1 += 0.035f * (brown * swell - lp1);
                lp2 += 0.025f * (lp1 - lp2);
                float sample = lp2 * 9500.0f;

                if (drop_env <= 0.001f && (rand() % 18000 == 0)) {
                    drop_freq  = 450.0f + (float)(rand() % 550);
                    drop_env   = 1.0f;
                    drop_phase = 0.0f;
                }

                if (drop_env > 0.001f) {
                    drop_freq  += 0.04f;
                    drop_phase += drop_freq / (float)sample_rate;
                    sample += sinf(drop_phase * 6.283185f) * drop_env * 650.0f;
                    drop_env *= 0.9975f;
                }

                if (sample > 32000.0f)  sample = 32000.0f;
                if (sample < -32000.0f) sample = -32000.0f;
                buffer[i] = (short)sample;
            }

            if (fwrite(buffer, sizeof(short), 1024, pipe) != 1024 || fflush(pipe) != 0) break;
        }

        pclose(pipe);
        _exit(0);
    } else {
        setpgid(pid, pid);
        g_audio_pid = pid;
        sigprocmask(SIG_SETMASK, &old_mask, NULL);
    }
}

static void stop_halal_ocean_ambience(void) {
    if (g_audio_pid <= 0) return;

    pid_t wp;
    while ((wp = waitpid(g_audio_pid, NULL, WNOHANG)) < 0 && errno == EINTR) {
    }
    if (wp == 0) {
        if (kill(-g_audio_pid, SIGTERM) != 0) {
            kill(g_audio_pid, SIGTERM);
        }
        while (waitpid(g_audio_pid, NULL, 0) < 0 && errno == EINTR) {
        }
    }
    g_audio_pid = -1;
}
#else
static void start_halal_ocean_ambience(void) {
    fprintf(stderr, "c-reef: note: procedural ALSA audio (--sound) is Linux-only; running silently.\n");
}

static void stop_halal_ocean_ambience(void) {
}
#endif

/* =========================================================================
 * ENTRY POINT (main)
 * ========================================================================= */
static void usage(const char *prog) {
    printf("Usage: %s [options]\n\n"
           "Options:\n"
           "  -s, --sound   play procedural ocean ambience (Linux ALSA / aplay)\n"
           "  -h, --help    show this help\n\n"
           "Controls: q or Ctrl+C to quit.\n"
           "Environment: NO_COLOR=1 disables colors.\n", prog);
}

static void show_too_small(void) {
    char msg[96];
    int n = snprintf(msg, sizeof(msg),
                     "\x1b[2J\x1b[1;1Hc-reef: terminal too small (need %dx%d)", MIN_COLS, MIN_ROWS);
    if (n > 0) {
        size_t len = (n >= (int)sizeof(msg)) ? (sizeof(msg) - 1) : (size_t)n;
        write_all(msg, len);
    }
}

int main(int argc, char **argv) {
    const char *prog = (argc > 0 && argv[0]) ? argv[0] : "c-reef";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sound") == 0) {
            g_sound = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(prog);
            return 0;
        } else {
            fprintf(stderr, "c-reef: unknown option '%s'\nTry '%s --help'.\n", argv[i], prog);
            return 2;
        }
    }

    if (!is_stdout_tty()) {
        fprintf(stderr, "c-reef: stdout is not a terminal\n");
        return 1;
    }

    const char *no_color = getenv("NO_COLOR");
    if (no_color && no_color[0] != '\0') g_use_color = 0;

    srand((unsigned int)time(NULL));
    install_signal_handlers();

    int buf_rc = allocate_buffers();
    if (buf_rc == 0) {
        fprintf(stderr, "c-reef: terminal too small (need at least %dx%d)\n", MIN_COLS, MIN_ROWS);
        return 1;
    }
    if (buf_rc < 0) {
        fprintf(stderr, "c-reef: out of memory\n");
        return 1;
    }
    g_buffers_ok = 1;
#ifdef _WIN32
    g_last_term_w = g_width;
    g_last_term_h = g_height;
#endif

    populate_aquarium();
    if (g_sound) start_halal_ocean_ambience();
    setup_terminal();

    const char *init_seq = "\x1b[?1049h\x1b[?7l\x1b[?25l\x1b[2J";
    write_all(init_seq, strlen(init_seq));

    FrameTimer timer;
    frame_timer_init(&timer);

    int exit_code = 0;
    while (g_running) {
        poll_resize();
        if (g_resized) {
            g_resized = 0;
            free_buffers();
            buf_rc = allocate_buffers();
            if (buf_rc > 0) {
                g_buffers_ok = 1;
                populate_aquarium();
                write_all("\x1b[2J", 4);
            } else if (buf_rc == 0) {
                g_buffers_ok = 0;
                show_too_small();
            } else {
                g_buffers_ok = 0;
                exit_code = 1;
                g_running = 0;
                break;
            }
        }

        poll_input();
        if (!g_running) break;

        if (g_buffers_ok) {
            update_simulation();
            render_frame();
            g_tick++;
        }

        if (!g_running) break;
        frame_timer_wait(&timer);
    }

    stop_halal_ocean_ambience();
    restore_terminal_input();

    const char *exit_seq = "\x1b[0m\x1b[?7h\x1b[?25h\x1b[?1049l\r\n";
    write_all(exit_seq, strlen(exit_seq));

    restore_terminal_output();
    free_buffers();
    return exit_code;
}
