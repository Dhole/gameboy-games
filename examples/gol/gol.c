#include <stdio.h>
#include <gb/rand.h>
#include <gb/font.h>
#include <gb/console.h>
#include <gb/drawing.h>

#define STAT_REG_HBLANK_IFLAG 0x08
#define STAT_REG_VBLANK_IFLAG 0x10
#define STAT_REG_OAM_IFLAG    0x20
#define STAT_REG_LYC_LY_IFLAG 0x40

#define STATE_CONTINUE 0
#define STATE_RESET    1
#define STATE_GLIDER   2
#define STATE_PAUSE    4

#define GRID_H 16
#define GRID_W 16

//#define P(x,y) (y*GRID_W + x)
#define P(x,y) ((y << 4) + x)

//UINT8 grid[2][GRID_H][GRID_W];
UINT8 grid_ab[2 * GRID_H * GRID_W];
UINT8 *grid;
UINT8 slot;
UINT8 frame_done;
UINT8 state;

const UINT8 cell_tiles[] = {
    // 0x00
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    // 0x01
    0x00,0x00,0x3C,0x3C,0x76,0x6E,0x62,0x5E,0x62,0x5E,0x66,0x7E,0x3C,0x3C,0x00,0x00
};

void
memset(UINT8 *p, UINT8 v, UINT16 len)
{
    UINT16 i;
    for (i = 0; i < len; i++) {
	p[i] = v;
    }
}

void
grid_init(void)
{
    slot = 0;
    memset(grid_ab, 0, 2*GRID_H*GRID_W*sizeof(UINT8));
    grid = grid_ab;
}

void
grid_set_glider(void)
{
    grid[P(1,0)] = 1;
    grid[P(2,1)] = 1;
    grid[P(0,2)] = 1;
    grid[P(1,2)] = 1;
    grid[P(2,2)] = 1;
}

void
grid_set_rand(void)
{
    UINT8 x, y;

    initrand(DIV_REG);
    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            grid[P(x,y)] = _rand() & 0x01;
        }
    }
}

void
grid_advance(void)
{
    UINT8 x, y, xb, ya, yb;
    UINT8 col0, col1, col2;
    UINT8 neighs, cell;
    //UINT8 slot2;
    UINT8 *grid0;

    if (slot == 0) {
	slot = 1;
	grid0 = grid_ab;
	grid = &grid_ab[1 * GRID_H * GRID_W];
    } else {
	slot = 0;
	grid = grid_ab;
	grid0 = &grid_ab[1 * GRID_H * GRID_W];
    }

    x = 0;
#define XA ((0-1)&0x0f)
#define X  ((0-0)&0x0f)
#define XB ((0+1)&0x0f)
    for (y = 0; y < GRID_H; y++) {
//#define XA 15
//#define XB 1
	ya = (y-1)&0x0f;
	yb = (y+1)&0x0f;
	col0 =  grid0[P(XA,ya)] +
		grid0[P(XA,y )] +
		grid0[P(XA,yb)];
	col1 =  grid0[P(X ,ya)] +
		grid0[P(X ,y )] +
		grid0[P(X ,yb)];
	col2 =  grid0[P(XB,ya)] +
		grid0[P(XB,y )] +
		grid0[P(XB,yb)];
	cell = grid0[P(X,y)];
	neighs = col0 + col1 + col2 - cell;
	if (neighs == 3) {
	    grid[P(X,y)] = 1;
	} else if (neighs == 2) {
	    grid[P(X,y)] = cell;
	} else {
	    grid[P(X,y)] = 0;
	}
	for (x = 1; x < GRID_W; x++) {
	    //xa = (x-1)&0x0f;
	    xb = (x+1)&0x0f;
	    col0 = col1;
	    col1 = col2;
	    // NOTE: in P(x, y), y << 4 is evaluated at every iteration.  We
	    // could save that value in the y-loop.
	    col2 =  grid0[P(xb,ya)] +
		    grid0[P(xb,y )] +
		    grid0[P(xb,yb)];
	    cell = grid0[P(x,y)];
	    neighs = col0 + col1 + col2 - cell;
	    if (neighs == 3) {
		grid[P(x,y)] = 1;
	    } else if (neighs == 2) {
		grid[P(x,y)] = cell;
	    } else {
		grid[P(x,y)] = 0;
	    }
	}
    }

}

volatile UINT8 frame;
UINT8 wave_flag, pause_flag;
UINT8 keys_prev;

void
vbl_int(void)
{
    UINT8 keys, keys_all;

    keys_all = joypad();
    keys = (keys_prev ^ keys_all) & keys_all;
    if (frame_done && (state != STATE_PAUSE)) {
	frame_done = 0;
	set_bkg_tiles(2, 1, 16, 16, grid);
    }
    if (keys & J_SELECT) {
	wave_flag = !wave_flag;
	if (wave_flag) {
	    STAT_REG |= STAT_REG_HBLANK_IFLAG;
	} else {
	    STAT_REG &= ~STAT_REG_HBLANK_IFLAG;
	    SCX_REG = 0;
	}
    } else if (keys & J_START) {
	pause_flag = !pause_flag;
	if (pause_flag) {
	    state = STATE_PAUSE;
	} else {
	    state = STATE_CONTINUE;
	}
    }
    if (!pause_flag) {
	if (keys & J_A) {
	    state = STATE_RESET;
	} else if (keys & J_B) {
	    state = STATE_GLIDER;
	}
    }
    keys_prev = keys_all;
    frame++;
}

signed char sin[] =
//{0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,0,0,0,0,0,0,0,0,0};
{0,0,0,1,1,1,2,2,3,3,3,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,6,6,6,6,5,5,5,5,4,4,4,3,3,3,2,2,1,1,1,0,0,0,0,0,-1,-1,-1,-2,-2,-3,-3,-3,-4,-4,-4,-5,-5,-5,-5,-6,-6,-6,-6,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-6,-6,-6,-6,-5,-5,-5,-5,-4,-4,-4,-3,-3,-3,-2,-2,-1,-1,-1,0,0,0,0,0,1,1,1,2,2,3,3,3,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,6,6,6,6,5,5,5,5,4,4,4,3,3,3,2,2,1,1,1,0,0,0,0,0,-1,-1,-1,-2,-2,-3,-3,-3,-4,-4,-4,-5,-5,-5,-5,-6,-6,-6,-6,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-7,-6,-6,-6,-6,-5,-5,-5,-5,-4,-4,-4,-3,-3,-3,-2,-2,-1,-1,-1,0,0};

void
lcd_int(void)
{
    SCX_REG = sin[(UINT8) (LY_REG + frame)];
}

font_t ibm_font;

void
intro(void)
{
    INT8 x, y;

    wait_vbl_done();
    printf("    Game of Life\n      by Dhole");

    for (y = 0; y < 80; y++) {
	SCY_REG = 16 - y;
	wait_vbl_done();
	wait_vbl_done();
    }
    for (y = 0; y < 40; y++) {
	wait_vbl_done();
    }
    for (x = 0; x < 32; x++) {
	set_bkg_tiles(x, 0, 1, 1, &cell_tiles[0]);
	set_bkg_tiles(x, 1, 1, 1, &cell_tiles[0]);
    }
    SCY_REG = 0;
}

void main(void)
{
    //UINT8 x, y;

    /* First, init the font system */
    font_init();

    /* Load all the fonts that we can */
    ibm_font = font_load(font_ibm);  /* 96 tiles */

    /* Load this one with dk grey background and white foreground */
    //color(WHITE, DKGREY, SOLID);

    intro();

    set_bkg_data(0x00, 0x02, cell_tiles);
    set_sprite_prop(0, 0x00);

    grid_init();
    //grid_set_glider();
    //grid_set_rand();

    wave_flag = 0;
    frame_done = 0;
    frame = 0;
    state = STATE_CONTINUE;
    //state = STATE_PAUSE;
    add_VBL(vbl_int);
    add_LCD(lcd_int);

    set_interrupts(VBL_IFLAG | LCD_IFLAG);
    //STAT_REG |= STAT_REG_HBLANK_IFLAG;
    enable_interrupts();

    while (1) {
	while (frame_done);
	switch (state) {
	case STATE_CONTINUE:
	    grid_advance();
	    break;
	case STATE_RESET:
	    grid_set_rand();
	    state = STATE_CONTINUE;
	    break;
	case STATE_GLIDER:
	    grid_set_glider();
	    state = STATE_CONTINUE;
	    break;
	case STATE_PAUSE:
	    break;
	}
	frame_done = 1;
    }
}

