#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <time.h>
#include <math.h>

/* =========================================================================
 * CONSTANTES & PALETTE ANSI
 * ========================================================================= */
#define MAX_ENTITIES 80
#define MAX_BUBBLES  120
#define MAX_KELP     16
#define FPS          30
#define FRAME_NS     (1000000000L / FPS)

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
    COL_ROCK          /* \x1b[90m - Dim Gray */
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
 * DÉFINITION EXACTE DES SPRITES (GÉOMÉTRIE ET SYMÉTRIE PRÉSERVÉES)
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
 * STRUCTURES DE DONNÉES
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
 * ÉTAT GLOBAL & BUFFERS
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
static long   g_tick = 0;
static pid_t  g_audio_pid = -1;

/* =========================================================================
 * UTILITAIRES & GESTION DES SIGNAUX POSIX
 * ========================================================================= */
static void handle_sig(int sig) {
    (void)sig;
    g_running = 0;
}

static void handle_winch(int sig) {
    (void)sig;
    g_resized = 1;
}

static float randf(float min_v, float max_v) {
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

static void allocate_buffers(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 20 && ws.ws_row > 10) {
        g_width  = ws.ws_col;
        g_height = ws.ws_row;
    } else {
        g_width  = 80;
        g_height = 24;
    }

    g_char_buf = malloc(sizeof(char *) * g_height);
    g_col_buf  = malloc(sizeof(unsigned char *) * g_height);
    for (int r = 0; r < g_height; r++) {
        g_char_buf[r] = malloc(g_width);
        g_col_buf[r]  = malloc(g_width);
    }

    g_sand_row1 = malloc(g_width);
    g_sand_row2 = malloc(g_width);
    g_sand_col1 = malloc(g_width);
    g_sand_col2 = malloc(g_width);

    g_out_buf_cap = (size_t)g_width * (size_t)g_height * 24 + 16384;
    g_out_buf = malloc(g_out_buf_cap);

    init_seabed();
}

/* =========================================================================
 * RENDU DES SPRITES AVEC TRANSPARENCE & CLIPPING STRICT
 * ========================================================================= */
#define DEGREE_TOKEN ((char)-1)

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
            draw_y += (int)roundf(sinf(e->phase + r * 1.4f) * 0.6f);
        }
        draw_sprite_line(draw_y, draw_x, sprite[r], col, e->type);
    }
}

/* =========================================================================
 * SPAWN & PHYSIQUE DES ENTITÉS
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
    float base_x = initial ? randf(5.0f, (float)(g_width - 15))
    : (dir > 0 ? -8.0f : (float)(g_width + 4));
    float base_y = randf(2.0f, (float)(g_height * 0.55f));
    float speed = randf(0.85f, 1.25f) * (float)dir;
    float shared_phase = randf(0.0f, 6.28f);

    for (int i = 0; i < pack_size; i++) {
        int idx = find_free_slot();
        if (idx < 0) break;
        Entity *e = &g_entities[idx];
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

    switch (type) {
        case SPECIES_GUPPY:
            e->vx = randf(0.45f, 0.70f) * e->dir;
            e->y  = randf(2.0f, (float)(sea_bottom - 8));
            margin = 10;
            break;
        case SPECIES_PUFFER:
            e->vx = randf(0.18f, 0.30f) * e->dir;
            e->y  = randf(3.0f, (float)(sea_bottom - 9));
            margin = 12;
            break;
        case SPECIES_ANGEL:
            e->vx = randf(0.28f, 0.45f) * e->dir;
            e->y  = randf((float)(g_height * 0.2f), (float)(g_height * 0.65f));
            margin = 10;
            break;
        case SPECIES_CLOWN:
            e->vx = randf(0.32f, 0.52f) * e->dir;
            e->y  = randf((float)(g_height * 0.25f), (float)(sea_bottom - 7));
            margin = 18;
            break;
        case SPECIES_EEL:
            e->vx = randf(0.35f, 0.55f) * e->dir;
            e->y  = randf((float)(g_height * 0.68f), (float)(sea_bottom - 4));
            if (e->y < 3.0f) e->y = 3.0f;
            margin = 24;
        break;
        case SPECIES_SWORD:
            e->vx = randf(0.75f, 1.10f) * e->dir;
            e->y  = randf(2.0f, (float)(g_height * 0.55f));
            margin = 20;
            break;
        case SPECIES_FLOUNDER:
            e->vx = randf(0.22f, 0.38f) * e->dir;
            e->y  = (float)(sea_bottom - 4);
            if (e->y < 2.0f) e->y = 2.0f;
            margin = 18;
        break;
        case SPECIES_TUNA:
            e->vx = randf(0.65f, 0.95f) * e->dir;
            e->y  = randf(3.0f, (float)(g_height * 0.60f));
            margin = 25;
            break;
        case SPECIES_SHARK:
            e->vx = randf(0.22f, 0.34f) * e->dir;
            e->y  = randf((float)(g_height * 0.30f), (float)(g_height * 0.62f));
            margin = 30;
            break;
        case SPECIES_SQUID:
            e->vx = randf(0.18f, 0.32f) * e->dir;
            e->vy = -0.35f;
            e->y  = randf((float)(g_height * 0.35f), (float)(sea_bottom - 9));
            e->anim_timer = 20 + (rand() % 25);
            margin = 16;
            break;
        case SPECIES_CRAB:
            e->vx = randf(0.12f, 0.22f) * e->dir;
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
        e->x = randf(2.0f, (float)(g_width - 15));
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
 * PIPELINE DE RENDU DOUBLE-BUFFER EN MÉMOIRE (POSIX / LINUX)
 * ========================================================================= */
static void render_frame(void) {
    for (int r = 0; r < g_height; r++) {
        memset(g_char_buf[r], ' ', g_width);
        memset(g_col_buf[r], COL_RESET, g_width);
    }

    int sea_bottom = g_height - 2;

    if (g_height >= 2) {
        memcpy(g_char_buf[g_height - 2], g_sand_row1, g_width);
        memcpy(g_col_buf[g_height - 2],  g_sand_col1, g_width);
        memcpy(g_char_buf[g_height - 1], g_sand_row2, g_width);
        memcpy(g_col_buf[g_height - 1],  g_sand_col2, g_width);
    }

    const char kelp_glyphs[] = "(){}";
    for (int i = 0; i < g_num_kelps; i++) {
        for (int h = 0; h < g_kelps[i].height; h++) {
            int row = sea_bottom - 1 - h;
            if (row < 1 || row >= sea_bottom) continue;
            float sway = sinf((float)g_tick * 0.08f + g_kelps[i].phase + (float)h * 0.45f);
            int col = g_kelps[i].x + (int)roundf(sway * 1.15f);
            if (col >= 0 && col < g_width) {
                char kc = kelp_glyphs[(h + (int)(g_tick / 12)) % 4];
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
        int n = snprintf(g_out_buf + pos, 16, "\x1b[%d;1H", r + 1);
        if (n > 0) pos += (size_t)n;

        int safe_cols = (g_width > 1) ? (g_width - 1) : g_width;
        for (int c = 0; c < safe_cols; c++) {
            unsigned char cell_col = g_col_buf[r][c];
            char ch = g_char_buf[r][c];

            if (cell_col != cur_col) {
                const char *ansi = ANSI_TABLE[cell_col];
                size_t len = strlen(ansi);
                memcpy(g_out_buf + pos, ansi, len);
                pos += len;
                cur_col = cell_col;
            }

            if (ch == DEGREE_TOKEN) {
                g_out_buf[pos++] = '\xc2';
                g_out_buf[pos++] = '\xb0';
            } else {
                g_out_buf[pos++] = ch;
            }
        }
    }

    const char *rst = ANSI_TABLE[COL_RESET];
    size_t rlen = strlen(rst);
    memcpy(g_out_buf + pos, rst, rlen);
    pos += rlen;

    ssize_t unused = write(STDOUT_FILENO, g_out_buf, pos);
    (void)unused;
}

/* =========================================================================
 * MOTEUR AUDIO PROCÉDURAL HALAL (ALSA / APLAY)
 * ========================================================================= */
static void start_halal_ocean_ambience(void) {
    pid_t pid = fork();
    if (pid < 0) return;

    if (pid == 0) {
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

            if (fwrite(buffer, sizeof(short), 1024, pipe) != 1024) break;
            fflush(pipe);
        }

        pclose(pipe);
        _exit(0);
    } else {
        g_audio_pid = pid;
    }
}

static void stop_halal_ocean_ambience(void) {
    if (g_audio_pid > 0) {
        kill(g_audio_pid, SIGTERM);
        waitpid(g_audio_pid, NULL, 0);
        g_audio_pid = -1;
    }
}

/* =========================================================================
 * POINT D'ENTRÉE PRINCIPAL (main)
 * ========================================================================= */
int main(void) {
    srand((unsigned int)time(NULL));

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sig;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    struct sigaction sa_winch;
    memset(&sa_winch, 0, sizeof(sa_winch));
    sa_winch.sa_handler = handle_winch;
    sigaction(SIGWINCH, &sa_winch, NULL);

    allocate_buffers();
    populate_aquarium();
    start_halal_ocean_ambience();

    const char *init_seq = "\x1b[?1049h\x1b[?7l\x1b[?25l\x1b[2J";
    ssize_t u1 = write(STDOUT_FILENO, init_seq, strlen(init_seq));
    (void)u1;

    struct timespec ts;
    ts.tv_sec  = 0;
    ts.tv_nsec = FRAME_NS;

    while (g_running) {
        if (g_resized) {
            g_resized = 0;
            free_buffers();
            allocate_buffers();
            populate_aquarium();
            ssize_t u_clr = write(STDOUT_FILENO, "\x1b[2J", 4);
            (void)u_clr;
        }

        update_simulation();
        render_frame();

        g_tick++;
        nanosleep(&ts, NULL);
    }

    stop_halal_ocean_ambience();

    const char *exit_seq = "\x1b[0m\x1b[?7h\x1b[?25h\x1b[?1049l\r\n";
    ssize_t u2 = write(STDOUT_FILENO, exit_seq, strlen(exit_seq));
    (void)u2;

    free_buffers();
    return 0;
}
