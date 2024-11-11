/***************************************************************
                          stage_game.c
                               
The main game
***************************************************************/

#include <nusys.h>
#include "config.h"
#include "netlib.h"
#include "packets.h"
#include "helper.h"
#include "text.h"

#include "assets/spr_board.h"
#include "assets/spr_cross.h"
#include "assets/spr_cross_large.h"
#include "assets/spr_circle.h"
#include "assets/spr_circle_large.h"
#include "assets/spr_tie_large.h"

static void board_render();

// Controller data
static NUContData global_contdata;

// Game data
static u8 global_playerturn;
static u8 global_gamestate;
static u8 global_gamestate_large[3][3];
static u8 global_gamestate_small[9][3][3];
static u8 global_forcedboard;
static u8 global_selectionx;
static u8 global_selectiony;


/*==============================
    refresh_lobbytext
    Refreshes the text on the screen with the game status
==============================*/

static void refresh_gametext()
{
    text_cleanup();
    text_setalign(ALIGN_CENTER);
    text_setcolor(255, 255, 255, 255);
    
    // Large text
    text_setfont(&font_default);
    if (global_gamestate == GAMESTATE_PLAYING && global_playerturn > 0)
    {
        if (global_playerturn == netlib_getclient())
            text_create("Your turn", SCREEN_WD/2, SCREEN_HT/2 + 64);
        else
            text_create("Opponent's turn", SCREEN_WD/2, SCREEN_HT/2 + 64);
    }
    else if (global_gamestate > GAMESTATE_PLAYING)
        text_create("Game Over", SCREEN_WD/2, SCREEN_HT/2 + 64);
        
    // Small text
    text_setfont(&font_small);
    if (global_gamestate == GAMESTATE_PLAYING && global_forcedboard != 0)
        text_create("Forced to play in highlighted board", SCREEN_WD/2, SCREEN_HT/2 + 64 + 24);
    else if (global_gamestate == GAMESTATE_ENDED_DISCONNECT)
        text_create("Player disconnected", SCREEN_WD/2, SCREEN_HT/2 + 64 + 24);
    else if (global_gamestate == GAMESTATE_ENDED_TIE)
        text_create("Tie game", SCREEN_WD/2, SCREEN_HT/2 + 64 + 24);
    else if ((netlib_getclient() == 1 && global_gamestate == GAMESTATE_ENDED_WINNER_1) || (netlib_getclient() == 2 && global_gamestate == GAMESTATE_ENDED_WINNER_2))
        text_create("You win!", SCREEN_WD/2, SCREEN_HT/2 + 64 + 24);
    else if (global_gamestate == GAMESTATE_ENDED_WINNER_1 || global_gamestate == GAMESTATE_ENDED_WINNER_2)
        text_create("Opponent wins!", SCREEN_WD/2, SCREEN_HT/2 + 64 + 24);
}


/*==============================
    stage_game_init
    Initialize the stage
==============================*/

void stage_game_init(void)
{
    global_playerturn = 0;
    global_forcedboard = 0;
    global_gamestate = GAMESTATE_PLAYING;
    global_selectionx = 4;
    global_selectiony = 4;
    memset(global_gamestate_large, 0, sizeof(u8)*3*3);
    memset(global_gamestate_small, 0, sizeof(u8)*3*3*9);
    refresh_gametext();
}


/*==============================
    stage_game_update
    Update stage variables every frame
==============================*/

void stage_game_update(void)
{
    nuContDataGetEx(&global_contdata, 0);
    if (global_playerturn == netlib_getclient())
    {
        if (global_contdata.trigger & L_JPAD)
        {
            if (global_selectionx == 0)
                global_selectionx = 8;
            else
                global_selectionx--;
        }
        if (global_contdata.trigger & R_JPAD)
        {
            if (global_selectionx == 8)
                global_selectionx = 0;
            else
                global_selectionx++;
        }
        if (global_contdata.trigger & U_JPAD)
        {
            if (global_selectiony == 0)
                global_selectiony = 8;
            else
                global_selectiony--;
        }
        if (global_contdata.trigger & D_JPAD)
        {
            if (global_selectiony == 8)
                global_selectiony = 0;
            else
                global_selectiony++;
        }
        if (global_contdata.trigger & A_BUTTON)
        {
            u8 ibx = global_selectionx/3;
            u8 iby = global_selectiony/3;
            u8 ix = global_selectionx%3;
            u8 iy = global_selectiony%3;
            u8 ib = ibx + iby*3;
            if (global_gamestate_small[ib][ix][iy] == 0)
            {
                global_gamestate_small[ib][ix][iy] = netlib_getclient();
                netlib_start(PACKETID_PLAYERMOVE);
                    netlib_writebyte(ibx);
                    netlib_writebyte(iby);
                    netlib_writebyte(ix);
                    netlib_writebyte(iy);
                netlib_sendtoserver();
                global_playerturn = 0;
                refresh_gametext();
            }
        }
    }
}


/*==============================
    stage_game_draw
    Draw the stage
==============================*/

void stage_game_draw(void)
{
    glistp = glist;

    // Initialize the RCP and framebuffer
    rcp_init();
    fb_clear(0, 0, 0);
    
    // Render the board
    board_render();

    // Render some text
    text_render();

    // Finish
    gDPFullSync(glistp++);
    gSPEndDisplayList(glistp++);
    nuGfxTaskStart(glist, (s32)(glistp - glist) * sizeof(Gfx), NU_GFX_UCODE_F3DEX, NU_SC_SWAPBUFFER);
}

static void board_render()
{
    int i, ix, iy, ib;
    const int board_topx = 320/2 - 152/2;
    const int board_topy = 240/2 - 152/2 - 16;
    
    // Set fill rect drawing mode
    gDPSetCycleType(glistp++, G_CYC_FILL);
    
    // Show the forced area
    if (global_forcedboard > 0)
    {
        const int w = 40, h = 40;
        const int padding_small = 2;
        const int padding_large = 12;
        const int x = board_topx + 4 + (w + padding_large)*((global_forcedboard-1)%3);
        const int y = board_topy + 4 + (h + padding_large)*((global_forcedboard-1)/3);
        if (global_playerturn == 1)
        {
            gDPSetFillColor(glistp++, GPACK_RGBA5551(64, 0, 0, 1)<<16 | GPACK_RGBA5551(64, 0, 0, 1));
        }
        else
        {
            gDPSetFillColor(glistp++, GPACK_RGBA5551(0, 0, 64, 1)<<16 | GPACK_RGBA5551(0, 0, 64, 1));
        }
        gDPFillRectangle(glistp++, x, y, x + w, y + h);
    }
    
    // Show your cursor location
    if (global_playerturn == netlib_getclient())
    {
        const int padding_small = 2;
        const int padding_large = 10;
        const int w = 12, h = 12;
        const int x = board_topx + 4 + (w + padding_small)*global_selectionx + padding_large*(global_selectionx/3);
        const int y = board_topy + 4 + (h + padding_small)*global_selectiony + padding_large*(global_selectiony/3);
        static int selection_alpha = 64;
        static int selection_state = 1;
        
        // Handle cursor blinking
        if ((selection_state == 1 && selection_alpha == 128) || (selection_state == -1 && selection_alpha == 64))
            selection_state = -selection_state;
        selection_alpha += selection_state*2;
        
        // Render the cursor
        gDPSetFillColor(glistp++, GPACK_RGBA5551(selection_alpha, selection_alpha, 0, 1)<<16 | GPACK_RGBA5551(selection_alpha, selection_alpha, 0, 1));
        gDPFillRectangle(glistp++, x, y, x + w, y + h);
    }
    
    // Set sprite drawing mode
    gDPSetCycleType(glistp++, G_CYC_1CYCLE);
    gDPSetRenderMode(glistp++, G_RM_ZB_XLU_SURF, G_RM_ZB_XLU_SURF2);
    gDPSetDepthSource(glistp++, G_ZS_PRIM);
    gDPSetPrimDepth(glistp++, 0, 0);
    gDPSetTexturePersp(glistp++, G_TP_NONE);
    gDPSetPrimColor(glistp++,0, 0, 255, 255, 255, 255); 
    gDPSetCombineMode(glistp++, G_CC_DECALRGBA, G_CC_DECALRGBA);
    gDPPipeSync(glistp++);
    
    // Draw the board
    for (i = 0; i < 152; i++)
    {
        const int w = 152, h = 152;
        const int x = board_topx;
        const int y = board_topy;
        gDPLoadMultiTile_4b(glistp++, spr_board, 0, G_TX_RENDERTILE, G_IM_FMT_IA, w, h, 0, i, w - 1, i, 0, G_TX_WRAP, G_TX_WRAP, 0, 0, G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(glistp++, x << 2, y + i << 2, x + w << 2, y + i+1 << 2,  G_TX_RENDERTILE,  0 << 5, i << 5,  1 << 10, 1 << 10);
    }
    
    // Draw completed boards
    for (iy = 0; iy < 3; iy++)
    {
        for (ix = 0; ix < 3; ix++)
        {
            const int padding_small = 2;
            const int padding_large = 12;
            const int w = 40, h = 40;
            const int x = board_topx + 4 + (w + padding_large)*ix;
            const int y = board_topy + 4 + (h + padding_large)*iy;
            if (global_gamestate_large[ix][iy] == 1) 
            {
                gDPLoadTextureBlock(glistp++, spr_cross_large, G_IM_FMT_RGBA, G_IM_SIZ_16b, w, h, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                gSPTextureRectangle(glistp++, x << 2, y << 2, (x + w) << 2, (y + h) << 2, G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);
            }
            else if (global_gamestate_large[ix][iy] == 2) 
            {
                gDPLoadTextureBlock(glistp++, spr_circle_large, G_IM_FMT_RGBA, G_IM_SIZ_16b, w, h, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                gSPTextureRectangle(glistp++, x << 2, y << 2, (x + w) << 2, (y + h) << 2, G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);
            }
            else if (global_gamestate_large[ix][iy] == 3) 
            {
                gDPLoadTextureBlock(glistp++, spr_tie_large, G_IM_FMT_RGBA, G_IM_SIZ_16b, w, h, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                gSPTextureRectangle(glistp++, x << 2, y << 2, (x + w) << 2, (y + h) << 2, G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);
            }
        }
    }
    
    // Draw the moves
    for (ib = 0; ib < 9; ib++)
    {
        u8 ibx = ib%3;
        u8 iby = ib/3;
        if (global_gamestate_large[ibx][iby] != 0)
            continue;
        for (iy = 0; iy < 3; iy++)
        {
            for (ix = 0; ix < 3; ix++)
            {
                const int padding_small = 2;
                const int padding_large = 12;
                const int w = 16, h = 16;
                const int x = board_topx + 2 + (w + padding_small - 4)*ix + (40 + padding_large)*ibx;
                const int y = board_topy + 2 + (h + padding_small - 4)*iy + (40 + padding_large)*iby;
                if (global_gamestate_small[ib][ix][iy] == 1)
                {
                    gDPLoadTextureBlock(glistp++, spr_cross, G_IM_FMT_RGBA, G_IM_SIZ_16b, w, h, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                    gSPTextureRectangle(glistp++, x << 2, y << 2, (x + w) << 2, (y + h) << 2, G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);
                }
                else if (global_gamestate_small[ib][ix][iy] == 2)
                {
                    gDPLoadTextureBlock(glistp++, spr_circle, G_IM_FMT_RGBA, G_IM_SIZ_16b, w, h, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                    gSPTextureRectangle(glistp++, x << 2, y << 2, (x + w) << 2, (y + h) << 2, G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);
                }
            }
        }
    }
}


/*==============================
    stage_game_cleanup
    Cleans up memory used by the stage
==============================*/

void stage_game_cleanup(void)
{
    text_cleanup();
}


/*==============================
    stage_game_statechange
    Sets the game state
==============================*/

void stage_game_statechange()
{
    netlib_readbyte(&global_gamestate);
    refresh_gametext();
}


/*==============================
    stage_game_playerturn
    Sets who's turn it is
==============================*/

void stage_game_playerturn()
{
    netlib_readbyte(&global_playerturn);
    netlib_readbyte(&global_forcedboard);
    refresh_gametext();
}


/*==============================
    stage_game_makemove
    Handles the opposing player's move
==============================*/

void stage_game_makemove()
{
    u8 ply; 
    u8 board; 
    u8 posx;
    u8 posy;
    netlib_readbyte(&ply);
    netlib_readbyte(&board);
    netlib_readbyte(&posx);
    netlib_readbyte(&posy);
    global_gamestate_small[board-1][posx][posy] = ply;
}


/*==============================
    stage_game_boardcompleted
    Handles a board being completed
==============================*/

void stage_game_boardcompleted()
{
    u8 board;
    u8 state;
    netlib_readbyte(&board);
    netlib_readbyte(&state);
    global_gamestate_large[(board-1)%3][(board-1)/3] = state;
}