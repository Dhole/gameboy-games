#include <stdio.h>
#include <gb/rand.h>
#include <gb/font.h>
#include <gb/console.h>
#include <gb/drawing.h>

#define TRUE 1
#define FALSE 0

#define STAT_REG_HBLANK_IFLAG 0x08
#define STAT_REG_VBLANK_IFLAG 0x10
#define STAT_REG_OAM_IFLAG    0x20
#define STAT_REG_LYC_LY_IFLAG 0x40

#define GRID_W 20
#define GRID_H 18

#define MODW(x) mod_n(x, GRID_W)
#define MODH(y) mod_n(y, GRID_H)

//#define MODW(x) (x % GRID_W)
//#define MODH(y) (y % GRID_H)

//#define GRID_W 16
//#define GRID_H 16
//
//#define MODW(x) (x & 0x0f)
//#define MODH(y) (y & 0x0f)

#define _P(x,y) (y*GRID_W + x)
#define P(x,y) _P(MODW(x), MODH(y))
#define PP(p) P(p.x, p.y)
#define _PP(p) _P(p.x, p.y)

const UINT8 count[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24};

//
// Types
//

enum state {
    state_step = 0,
    state_pause,
    state_game_over,
};

enum cell {
    cell_snake_up = 0,
    cell_snake_down,
    cell_snake_left,
    cell_snake_right,
    cell_snake_head,
    cell_fruit,
    cell_empty,
};

enum direction {
    dir_up = 0,
    dir_down,
    dir_left,
    dir_right,
};

struct point {
    UINT8 x;
    UINT8 y;
};

struct game_state {
    UINT8 grid[GRID_H * GRID_W];
    struct point snake_head;
    struct point snake_head_prev;
    struct point snake_tail;
    struct point snake_tail_prev;
    struct point fruit;
    struct point fruit_prev;
    enum direction _dir;
    enum direction dir;
    enum direction dir_prev;
    UINT8 eating;
    UINT8 eating_prev;
    UINT16 score;
    UINT8 draw_text;
};

//
// Globals
//
font_t ibm_font;

volatile UINT8 frame_lcd;
UINT8 game_state_update, frame_done;
UINT8 redraw;
UINT8 state;
UINT8 keys_prev, keys;

struct game_state game;

//const UINT8 cell_tiles[] = {
//    // 0x00
//    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//    // 0x01
//    0x00,0x00,0x3C,0x3C,0x76,0x6E,0x62,0x5E,0x62,0x5E,0x66,0x7E,0x3C,0x3C,0x00,0x00
//};

enum tiles {
    tile_empty = 0,

    tile_tail_up,
    tile_tail_down,
    tile_tail_left,
    tile_tail_right,

    tile_body_left_right,
    tile_body_up_down,

    tile_body_right_up,
    tile_body_left_up,
    tile_body_right_down,
    tile_body_left_down,

    tile_head_up,
    tile_head_down,
    tile_head_left,
    tile_head_right,

    tile_fruit,
    tile_head_fruit_up,
    tile_head_fruit_down,
    tile_head_fruit_left,
    tile_head_fruit_right,

    tile_head_mouth_right,

    tile_body_left_right_ate,
    tile_body_up_down_ate,
};

const UINT8 tiles[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x3c,0x24,0x3c,0x24,0x14,0x1c,0x1c,0x14,0x1c,0x14,0x1c,0x14,0x08,0x08,0x00,0x00,
    0x00,0x00,0x10,0x10,0x38,0x28,0x38,0x28,0x38,0x28,0x28,0x38,0x34,0x2c,0x3c,0x24,
    0x00,0x00,0x00,0x00,0xc0,0xc0,0xfc,0x3c,0xbe,0x42,0xfc,0xfc,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0x03,0x3d,0x3e,0x7b,0x44,0x3f,0x3f,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0xff,0xff,0xdd,0x22,0xbb,0x44,0xff,0xff,0x00,0x00,0x00,0x00,
    0x3c,0x24,0x2c,0x34,0x34,0x2c,0x3c,0x24,0x3c,0x24,0x2c,0x34,0x34,0x2c,0x3c,0x24,
    0x3c,0x24,0x2c,0x34,0xf4,0xcc,0xdc,0x24,0xb8,0x48,0xf0,0xf0,0x00,0x00,0x00,0x00,
    0x3c,0x24,0x2c,0x34,0x37,0x2f,0x3d,0x22,0x1b,0x14,0x0f,0x0f,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0xf0,0xf0,0xd8,0x28,0xbc,0x44,0xec,0xf4,0x34,0x2c,0x3c,0x24,
    0x00,0x00,0x00,0x00,0x0f,0x0f,0x1d,0x12,0x3b,0x24,0x2f,0x37,0x34,0x2c,0x3c,0x24,
    0x38,0x38,0x6c,0x54,0x6c,0x54,0xbc,0x84,0xfc,0x84,0x7c,0x44,0x3c,0x24,0x3c,0x24,
    0x3c,0x24,0x3c,0x24,0x3e,0x22,0x3f,0x21,0x3d,0x21,0x36,0x2a,0x36,0x2a,0x1c,0x1c,
    0x18,0x18,0x6c,0x64,0xff,0x83,0x9f,0xe0,0xff,0x80,0x7f,0x7f,0x00,0x00,0x00,0x00,
    0x18,0x18,0x36,0x26,0xff,0xc1,0xf9,0x07,0xff,0x01,0xfe,0xfe,0x00,0x00,0x00,0x00,
    0x00,0x30,0x00,0x08,0x00,0x7e,0x00,0x5e,0x00,0x5e,0x00,0x6e,0x00,0x3c,0x00,0x00,
    0x00,0x30,0x00,0x08,0xc2,0xbe,0x6e,0x52,0xbe,0x82,0xfc,0x84,0x7c,0x44,0x3c,0x24,
    0x3c,0x24,0x3e,0x22,0x3f,0x21,0x7d,0x41,0x76,0x4a,0x63,0x4d,0x01,0x3d,0x00,0x00,
    0x2c,0x2c,0x36,0x12,0x1f,0x61,0x0f,0x50,0x1f,0x40,0x3f,0x47,0x38,0x38,0x00,0x00,
    0x34,0x34,0x6c,0x48,0xf8,0x86,0xf0,0x0e,0xf8,0x06,0xfc,0xe2,0x1c,0x1c,0x00,0x00,
    0x34,0x34,0x6c,0x48,0xf8,0x80,0xf0,0x00,0xf8,0x00,0xfc,0xe0,0x1c,0x1c,0x00,0x00,
    0x00,0x00,0x3c,0x3c,0xef,0xd3,0xdd,0x22,0xbb,0x44,0xf7,0xcb,0x3c,0x3c,0x00,0x00,
    0x3c,0x24,0x2c,0x34,0x76,0x4a,0x7a,0x46,0x5e,0x62,0x6e,0x52,0x34,0x2c,0x3c,0x24,
};

//
// Logic
//

// This function only works for values in the range [0..n-1] +/- 1
UINT8
mod_n(UINT8 x, UINT8 n)
{
    if (x == 0xff) {
        return n-1;
    } else if (x == n) {
        return 0;
    } else {
	return x;
    }
}

void
memset(UINT8 *p, UINT8 v, UINT16 len)
{
    UINT16 i;
    for (i = 0; i < len; i++) {
	p[i] = v;
    }
}

void
grid_clear(UINT8 *grid)
{
    memset(grid, cell_empty, GRID_H*GRID_W*sizeof(UINT8));
}

void
game_place_fruit_rand(void)
{
    UINT8 x, y;
    do {
	x = _rand() % GRID_W;
	y = _rand() % GRID_H;
    } while (game.grid[P(x,y)] != cell_empty);
    game.fruit_prev.x = game.fruit.x;
    game.fruit_prev.y = game.fruit.y;
    game.grid[P(x,y)] = cell_fruit;
    game.fruit.x = MODW(x);
    game.fruit.y = MODW(y);
}

void
game_state_init(void)
{
    grid_clear(game.grid);
    game.snake_tail.x = 2; game.snake_tail.y = 2;
    game.snake_tail_prev.x = game.snake_tail.x;
    game.snake_tail_prev.y = game.snake_tail.y;
    game.snake_head.x = 4; game.snake_head.y = 2;
    game.snake_head_prev.x = game.snake_head.x - 1;
    game.snake_head_prev.y = game.snake_head.y;
    game.grid[PP(game.snake_tail)] = cell_snake_right;
    game.grid[PP(game.snake_head_prev)] = cell_snake_right;
    game.grid[PP(game.snake_head)] = cell_snake_head;
    game._dir = dir_right;
    game.dir = game._dir;
    game.dir_prev = dir_right;
    game_place_fruit_rand();
    game.fruit_prev.x = game.fruit.x;
    game.fruit_prev.y = game.fruit.y;
    game.eating = FALSE;
    game.eating_prev = FALSE;
    game.score = 0;
    game.draw_text = FALSE;
    frame_done = TRUE;
}

void
game_state_step(void)
{

}

void
init(void)
{
    initrand(DIV_REG);
    redraw = TRUE;
    game_state_update = FALSE;
    frame_lcd = 0;
}


void
intro(void)
{
    INT8 x, y;

    // Init the font system
    font_init();
    // Load the fonts
    ibm_font = font_load(font_ibm);  // 96 tiles

    wait_vbl_done();
    printf("       Snake\n  a game by Dhole\n");

    for (y = 0; y < 80; y++) {
	SCY_REG = 16 - y;
	wait_vbl_done();
	wait_vbl_done();
    }
    for (y = 0; y < 40; y++) {
	wait_vbl_done();
    }
    for (x = 0; x < 32; x++) {
	set_bkg_tiles(x, 0, 1, 1, &count[tile_empty]);
	set_bkg_tiles(x, 1, 1, 1, &count[tile_empty]);
    }
    SCY_REG = 0;
}

inline void
game_step(void)
{
    if ((keys & J_START) && !(keys_prev & J_START)) {
	state = state_pause;
	game.draw_text = TRUE;
	return;
    }
    if (keys & J_UP) {
	if (game.dir != dir_down) {
	    game._dir = dir_up;
	}
    } else if (keys & J_DOWN) {
	if (game.dir != dir_up) {
	    game._dir = dir_down;
	}
    } else if (keys & J_LEFT) {
	if (game.dir != dir_right) {
	    game._dir = dir_left;
	}
    } else if (keys & J_RIGHT) {
	if (game.dir != dir_left) {
	    game._dir = dir_right;
	}
    }
    if ((frame_lcd % 8) != 0) {
	return;
    }
    game.dir_prev = game.dir;
    game.dir = game._dir;

    game.snake_head_prev.x = game.snake_head.x;
    game.snake_head_prev.y = game.snake_head.y;
    switch (game.dir) {
    case dir_up:
	game.grid[_PP(game.snake_head)] = cell_snake_up;
	game.snake_head.y = MODH(game.snake_head.y - 1);
	break;
    case dir_down:
	game.grid[_PP(game.snake_head)] = cell_snake_down;
	game.snake_head.y = MODH(game.snake_head.y + 1);
	break;
    case dir_left:
	game.grid[_PP(game.snake_head)] = cell_snake_left;
	game.snake_head.x = MODW(game.snake_head.x - 1);
	break;
    case dir_right:
	game.grid[_PP(game.snake_head)] = cell_snake_right;
	game.snake_head.x = MODW(game.snake_head.x + 1);
	break;
    }
    if (game.grid[_PP(game.snake_head)] == cell_fruit) {
	game.grid[_PP(game.snake_head)] = cell_snake_head;
	game_place_fruit_rand();
	game.eating = TRUE;
	game.score++;
	return;
    }
    game.eating_prev = game.eating;
    game.eating = FALSE;
    //if (game.grid[_PP(game.snake_head)] >= cell_snake_up &&
    if (game.grid[_PP(game.snake_head)] <= cell_snake_head) {
	state = state_game_over;
	game.draw_text = TRUE;
	return;
    }
    game.grid[_PP(game.snake_head)] = cell_snake_head;

    game.snake_tail_prev.x = game.snake_tail.x;
    game.snake_tail_prev.y = game.snake_tail.y;
    switch (game.grid[_PP(game.snake_tail)]) {
    case cell_snake_up:
	game.snake_tail.y = MODH(game.snake_tail.y - 1);
	break;
    case cell_snake_down:
	game.snake_tail.y = MODH(game.snake_tail.y + 1);
	break;
    case cell_snake_left:
	game.snake_tail.x = MODW(game.snake_tail.x - 1);
	break;
    case cell_snake_right:
	game.snake_tail.x = MODW(game.snake_tail.x + 1);
	break;
    }
    game.grid[_PP(game.snake_tail_prev)] = cell_empty;
}

inline void
game_pause(void)
{
    if ((keys & J_START) && !(keys_prev & J_START)) {
	state = state_step;
	return;
    }
}

inline void
game_over(void)
{
    if ((keys & J_START) && !(keys_prev & J_START)) {
	game_state_init();
	state = state_step;
	redraw = TRUE;
	set_bkg_data(0x00, sizeof(tiles) / 8, tiles);
	return;
    }
    if (game.draw_text) {
	// Init the font system
	font_init();
	// Load the fonts
	ibm_font = font_load(font_ibm);  // 96 tiles
	gotoxy(0, 5);
	printf("     Game Over\n\n      Score: %d\n\n   Press start to\n     play again",
	       game.score);
	game.draw_text = FALSE;
    }
}

inline void
main_loop(void)
{
    switch (state) {
    case state_step:
	game_step();
	break;
    case state_pause:
	game_pause();
	break;
    case state_game_over:
	game_over();
	break;
    }
}

//inline void
//draw(void)
//{
//    UINT8 x, y;
//    for (y = 0; y < GRID_H; y++) {
//        for (x = 0; x < GRID_W; x++) {
//	    if (game.grid[_P(x, y)]) {
//		set_bkg_tiles(x, y, 1, 1, &count[1]);
//	    } else {
//		set_bkg_tiles(x, y, 1, 1, &count[tile_empty]);
//	    }
//        }
//    }
//}

void
bkg_clear(void)
{
    UINT8 x, y;
    for (y = 0; y < GRID_H; y++) {
        for (x = 0; x < GRID_W; x++) {
	    set_bkg_tiles(x, y, 1, 1, &count[tile_empty]);
        }
    }
}

inline void
draw_update(void)
{
    UINT8 tile;

    //set_bkg_tiles(game.fruit_prev.x, game.fruit_prev.y, 1, 1, &count[0]);
    set_bkg_tiles(game.snake_tail_prev.x, game.snake_tail_prev.y, 1, 1, &count[tile_empty]);

    set_bkg_tiles(game.fruit.x, game.fruit.y, 1, 1, &count[tile_fruit]);

    tile = tile_tail_up + game.grid[_PP(game.snake_tail)];
    set_bkg_tiles(game.snake_tail.x, game.snake_tail.y, 1, 1, &count[tile]);

    switch (game.dir_prev) {
    case dir_up:
	switch (game.dir) {
	case dir_up:
	    tile = tile_body_up_down;
	    break;
	case dir_down:
	    tile = tile_body_up_down;
	    break;
	case dir_left:
	    tile = tile_body_right_down;
	    break;
	case dir_right:
	    tile = tile_body_left_down;
	    break;
	}
	break;
    case dir_down:
	switch (game.dir) {
	case dir_up:
	    tile = tile_body_up_down;
	    break;
	case dir_down:
	    tile = tile_body_up_down;
	    break;
	case dir_left:
	    tile = tile_body_right_up;
	    break;
	case dir_right:
	    tile = tile_body_left_up;
	    break;
	}
	break;
    case dir_left:
	switch (game.dir) {
	case dir_up:
	    tile = tile_body_left_up;
	    break;
	case dir_down:
	    tile = tile_body_left_down;
	    break;
	case dir_left:
	    tile = tile_body_left_right;
	    break;
	case dir_right:
	    tile = tile_body_left_right;
	    break;
	}
	break;
    case dir_right:
	switch (game.dir) {
	case dir_up:
	    tile = tile_body_right_up;
	    break;
	case dir_down:
	    tile = tile_body_right_down;
	    break;
	case dir_left:
	    tile = tile_body_left_right;
	    break;
	case dir_right:
	    tile = tile_body_left_right;
	    break;
	}
	break;
    }
    if (((tile >= tile_body_left_right) && (tile <= tile_body_up_down)) &&
       game.eating_prev) {
	tile += (tile_body_left_right_ate - tile_body_left_right);
    }
    set_bkg_tiles(game.snake_head_prev.x, game.snake_head_prev.y, 1, 1, &count[tile]);

    tile = tile_head_up + game.dir;
    if (game.eating) {
	tile += (tile_head_fruit_up - tile_head_up);
    }
    set_bkg_tiles(game.snake_head.x, game.snake_head.y, 1, 1, &count[tile]);
}

inline void
draw_start(void)
{
    bkg_clear();
    draw_update();
    //set_bkg_tiles(game.snake_tail.x, game.snake_tail.y, 1, 1, &count[tile_tail_right]);
    //set_bkg_tiles(game.snake_tail.x + 1, game.snake_tail.y, 1, 1, &count[tile_body_left_right]);
    //set_bkg_tiles(game.snake_head.x, game.snake_head.y, 1, 1, &count[tile_head_right]);
    //set_bkg_tiles(game.fruit.x, game.fruit.y, 1, 1, &count[tile_fruit]);
}

UINT8 keys_all;

void
vbl_int(void)
{
    frame_lcd++;
    if (!frame_done) {
	return;
    }
    keys_prev = keys;
    keys = joypad();
    if (game_state_update) {
	game_state_update = FALSE;
	switch (state) {
	case state_step:
	    if (redraw) {
		draw_start();
		redraw = FALSE;
	    } else {
		draw_update();
	    }
	    break;
	case state_pause:
	    break;
	case state_game_over:
	    break;
	}
	frame_done = TRUE;
    }
}

void
main(void)
{
    //HIDE_BKG;
    //HIDE_WIN;
    //HIDE_SPRITES;
    init();

    //intro();

    game_state_init();

    state = state_step;

    set_interrupts(VBL_IFLAG);
    enable_interrupts();

    set_bkg_data(0x00, sizeof(tiles) / 8, tiles);
    wait_vbl_done();
    draw_start();
    SHOW_BKG;
    add_VBL(vbl_int);
    while (1) {
	while (game_state_update);
	main_loop();
	game_state_update = TRUE;
    }
}
