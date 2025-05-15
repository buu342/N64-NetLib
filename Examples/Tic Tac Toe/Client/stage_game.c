/***************************************************************
                          stage_game.c
                               
The main game
***************************************************************/

#include <nusys.h>
#include <string.h> // For memset
#include "config.h"
#include "netlib.h"
#include "stages.h"
#include "packets.h"
#include "helper.h"
#include "text.h"

#include "assets/spr_board.h"
#include "assets/spr_cross.h"
#include "assets/spr_cross_large.h"
#include "assets/spr_circle.h"
#include "assets/spr_circle_large.h"
#include "assets/spr_tie_large.h"


/*********************************
        Function Prototypes
*********************************/

static void board_render();
static void refresh_gametext();
static void controller_movement();
static void cursor_toboard(u8 board);


/*********************************
             Globals
*********************************/

// Controller data
static NUContData global_contdata;

// Game data
static u8 global_playerturn;
static u8 global_gamestate;
static u8 global_gridstate_large[3][3];
static u8 global_gridstate_small[9][3][3];
static u8 global_forcedboard;
static u8 global_selectionx;
static u8 global_selectiony;
static s8 global_opponentx;
static s8 global_opponenty;


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
    global_opponentx = -1;
    global_opponenty = -1;
    memset((u8*)global_gridstate_large, 0, sizeof(u8)*3*3);
    memset((u8*)global_gridstate_small, 0, sizeof(u8)*3*3*9);
    refresh_gametext();
}


/*==============================
    stage_game_update
    Update stage variables every frame
==============================*/

void stage_game_update(void)
{
    static u8 couldplaybefore = FALSE;
    
    if (global_gamestate == GAMESTATE_PLAYING && global_playerturn == netlib_getclient())
    {       
        // Move the cursor to a decent place when the player is allowed to play
        if (!couldplaybefore)
        {
            u8 cbx = global_selectionx/3;
            u8 cby = global_selectiony/3;
            u8 isx = global_selectionx%3;
            u8 isy = global_selectiony%3;
            u8 fbx = (global_forcedboard-1)%3;
            u8 fby = (global_forcedboard-1)/3;
            
            // If we're forced to play a board, and the cursor isn't inside it, move the cursor into it
            if (global_forcedboard != 0 && (cbx != fbx || cby != fby))
            {
                cursor_toboard(global_forcedboard);
            }
            else if (global_gridstate_large[cbx][cby] != 0) // If the player's cursor is now inside a completed board, move it
            {
                // If the center board is complete, move it to the first non empty board.
                if (global_gridstate_large[1][1] != 0)
                {
                    u8 found = FALSE;
                    u8 tbx, tby;
                    for (tby = 0; tby < 3; tby++)
                    {
                        for (tbx = 0; tbx < 3; tbx++)
                        {
                            if (global_gridstate_large[tbx][tby] == 0)
                            {
                                cursor_toboard(1 + tbx + tby*3);
                                found = TRUE;
                                break;
                            }
                        }
                        if (found)
                            break;
                    }
                }
                else // Otherwise move to the center board
                    cursor_toboard(5);
            }
            // Otherwise, leave the cursor where the player last made a move
            
            // Network the current cursor position
            netlib_start(PACKETID_PLAYERCURSOR);
                netlib_writebyte(global_selectionx);
                netlib_writebyte(global_selectiony);
            netlib_broadcast();
            couldplaybefore = TRUE;
        }
        
        // Handle controller input
        controller_movement();
        if (global_contdata.trigger & A_BUTTON)
        {
            u8 cbx = global_selectionx/3;
            u8 cby = global_selectiony/3;
            u8 isx = global_selectionx%3;
            u8 isy = global_selectiony%3;
            u8 cb = cbx + cby*3;
            if (global_gridstate_small[cb][isx][isy] == 0)
            {
                global_gridstate_small[cb][isx][isy] = netlib_getclient();
                netlib_start(PACKETID_PLAYERMOVE);
                    netlib_writebyte(cbx);
                    netlib_writebyte(cby);
                    netlib_writebyte(isx);
                    netlib_writebyte(isy);
                netlib_sendtoserver();
                global_playerturn = 0;
                refresh_gametext();
            }
        }
    }
    else
        couldplaybefore = FALSE;
}


/*==============================
    cursor_toboard
    Move the cursor to a specific board
==============================*/

static void cursor_toboard(u8 board)
{
    u8 csx = global_selectionx%3;
    u8 csy = global_selectiony%3;
    u8 fbx = (board-1)%3;
    u8 fby = (board-1)/3;
    
    // If the middle one is empty, move there, otherwise find the most empty one
    if (global_gridstate_small[board-1][csx][csy] == 0)
    {
        global_selectionx = fbx*3 + 1;
        global_selectiony = fby*3 + 1;
    }
    else
    {
        u8 tsx, tsy;
        u8 found = FALSE;
        for (tsy = 0; tsy < 3; tsy++)
        {
            for (tsx = 0; tsx < 3; tsx++)
            {
                if (global_gridstate_small[board-1][tsx][tsy] == 0)
                {
                    global_selectionx = fbx*3 + tsx;
                    global_selectiony = fby*3 + tsy;
                    found = TRUE;
                    break;
                }
            }
            if (found)
                break;
        }
    }
}

/*==============================
    clamp
    Clamps a value between two numbers
    @param  The value to clamp
    @param  The minimum value
    @param  The maximum value
    @return The clamped value
==============================*/

static s8 clamp(s8 val, s8 min, s8 max)
{
    if (val < min)
        return max;
    if (val > max)
        return min;
    return val;
}

/*==============================
    controller_movement
    Handle controller movement with an input window
    to allow for diagonal movements
==============================*/

static void controller_movement()
{
    const u8 inputbuffer = 5;
    static u8 movingframes = 0;
    
    // Read controller data
    nuContDataGetEx(&global_contdata, 0);
    
    // Check any movement button was pressed
    if (global_contdata.button & (L_JPAD | R_JPAD | U_JPAD | D_JPAD))
        movingframes++;
    else
        movingframes = 0;
        
    // If pressed for long enough, we have movement
    if (movingframes == inputbuffer)
    {
        s8 tx = 0;
        s8 ty = 0;
        if ((global_contdata.button & L_JPAD) && (global_contdata.button & U_JPAD))
        {
            tx = -1;
            ty = -1;
        }
        else if ((global_contdata.button & L_JPAD) && (global_contdata.button & D_JPAD))
        {
            tx = -1;
            ty = 1;
        }
        else if ((global_contdata.button & R_JPAD) && (global_contdata.button & D_JPAD))
        {
            tx = 1;
            ty = 1;
        }
        else if ((global_contdata.button & R_JPAD) && (global_contdata.button & U_JPAD))
        {
            tx = 1;
            ty = -1;
        }
        else if (global_contdata.button & L_JPAD)
            tx = -1;
        else if (global_contdata.button & R_JPAD)
            tx = 1;
        else if (global_contdata.button & U_JPAD)
            ty = -1;
        else if (global_contdata.button & D_JPAD)
            ty = 1;
        
        // Move the cursor by the limit
        if (global_forcedboard > 0)
        {
            s8 fbx = (global_forcedboard-1)%3;
            s8 fby = (global_forcedboard-1)/3;
            s8 xlimit_min = fbx*3;
            s8 ylimit_min = fby*3;
            s8 xlimit_max = fbx*3 + 2;
            s8 ylimit_max = fby*3 + 2;
            global_selectionx = clamp(global_selectionx + tx, xlimit_min, xlimit_max);
            global_selectiony = clamp(global_selectiony + ty, ylimit_min, ylimit_max);
        }
        else
        {
            do
            {
                global_selectionx = clamp(global_selectionx + tx, 0, 8);
                global_selectiony = clamp(global_selectiony + ty, 0, 8);
            }
            while (global_gridstate_large[global_selectionx/3][global_selectiony/3] != 0);
        }
        
        // Network the current cursor position
        netlib_start(PACKETID_PLAYERCURSOR);
            netlib_setflags(FLAG_UNRELIABLE); // This packet isn't really important so no worries if it gets dropped
            netlib_writebyte(global_selectionx);
            netlib_writebyte(global_selectiony);
        netlib_broadcast();
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


/*==============================
    board_render
    Renders the game board
==============================*/

static void board_render()
{
    int i, ix, iy, ib;
    const int board_topx = 320/2 - 152/2;
    const int board_topy = 240/2 - 152/2 - 16;
    
    // Set fill rect drawing mode
    gDPSetCycleType(glistp++, G_CYC_FILL);
    
    // Show the forced area
    if (global_forcedboard > 0 && global_playerturn != 0)
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
    
    // Show the cursor location
    if (global_gamestate == GAMESTATE_PLAYING && (global_playerturn == netlib_getclient() || (global_opponentx != -1 && global_opponenty != -1)))
    {
        const int cursorx = (global_playerturn == netlib_getclient()) ? global_selectionx : global_opponentx;
        const int cursory = (global_playerturn == netlib_getclient()) ? global_selectiony : global_opponenty;
        const int padding_small = 2;
        const int padding_large = 10;
        const int w = 12, h = 12;
        const int x = board_topx + 4 + (w + padding_small)*cursorx + padding_large*(cursorx/3);
        const int y = board_topy + 4 + (h + padding_small)*cursory + padding_large*(cursory/3);
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
            if (global_gridstate_large[ix][iy] == 1) 
            {
                gDPLoadTextureBlock(glistp++, spr_cross_large, G_IM_FMT_RGBA, G_IM_SIZ_16b, w, h, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                gSPTextureRectangle(glistp++, x << 2, y << 2, (x + w) << 2, (y + h) << 2, G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);
            }
            else if (global_gridstate_large[ix][iy] == 2) 
            {
                gDPLoadTextureBlock(glistp++, spr_circle_large, G_IM_FMT_RGBA, G_IM_SIZ_16b, w, h, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                gSPTextureRectangle(glistp++, x << 2, y << 2, (x + w) << 2, (y + h) << 2, G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);
            }
            else if (global_gridstate_large[ix][iy] == 3) 
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
        if (global_gridstate_large[ibx][iby] != 0)
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
                if (global_gridstate_small[ib][ix][iy] == 1)
                {
                    gDPLoadTextureBlock(glistp++, spr_cross, G_IM_FMT_RGBA, G_IM_SIZ_16b, w, h, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                    gSPTextureRectangle(glistp++, x << 2, y << 2, (x + w) << 2, (y + h) << 2, G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);
                }
                else if (global_gridstate_small[ib][ix][iy] == 2)
                {
                    gDPLoadTextureBlock(glistp++, spr_circle, G_IM_FMT_RGBA, G_IM_SIZ_16b, w, h, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                    gSPTextureRectangle(glistp++, x << 2, y << 2, (x + w) << 2, (y + h) << 2, G_TX_RENDERTILE, 0 << 5, 0 << 5, 1 << 10, 1 << 10);
                }
            }
        }
    }
}


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
    if (global_playerturn != 0 && global_gamestate == GAMESTATE_PLAYING && global_forcedboard != 0)
        text_create("Forced to play in highlighted board", SCREEN_WD/2, SCREEN_HT/2 + 64 + 24);
    else if (global_gamestate == GAMESTATE_ENDED_DISCONNECT)
        text_create("Player disconnected", SCREEN_WD/2, SCREEN_HT/2 + 64 + 24);
    else if (global_gamestate == GAMESTATE_ENDED_TIE)
        text_create("Tie game", SCREEN_WD/2, SCREEN_HT/2 + 64 + 24);
    else if ((netlib_getclient() == 1 && global_gamestate == GAMESTATE_ENDED_WINNER_1) || (netlib_getclient() == 2 && global_gamestate == GAMESTATE_ENDED_WINNER_2))
        text_create("You win", SCREEN_WD/2, SCREEN_HT/2 + 64 + 24);
    else if (global_gamestate == GAMESTATE_ENDED_WINNER_1 || global_gamestate == GAMESTATE_ENDED_WINNER_2)
        text_create("Opponent wins", SCREEN_WD/2, SCREEN_HT/2 + 64 + 24);
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
    netlib_readbyte((u8*)&global_gamestate);
    if (global_gamestate == GAMESTATE_LOBBY)
    {
        stages_changeto(STAGE_LOBBY);
    }
    else if (global_gamestate >= GAMESTATE_ENDED_WINNER_1 || global_gamestate <= GAMESTATE_ENDED_DISCONNECT)
    {
        global_playerturn = 0;
        global_forcedboard = 0;
        global_opponentx = -1;
        global_opponenty = -1;
    }
    refresh_gametext();
}


/*==============================
    stage_game_playerturn
    Sets who's turn it is
==============================*/

void stage_game_playerturn()
{
    netlib_readbyte((u8*)&global_playerturn);
    netlib_readbyte((u8*)&global_forcedboard);
    refresh_gametext();
}


/*==============================
    stage_game_playercursor
    Handles the opposing player's move
==============================*/

void stage_game_playercursor()
{
    netlib_readbyte((s8*)&global_opponentx);
    netlib_readbyte((s8*)&global_opponenty);
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
    global_gridstate_small[board-1][posx][posy] = ply;
    global_opponentx = -1;
    global_opponenty = -1;
    global_playerturn = 0;
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
    global_gridstate_large[(board-1)%3][(board-1)/3] = state;
}