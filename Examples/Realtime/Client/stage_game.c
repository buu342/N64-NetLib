/***************************************************************
                          stage_game.c
                               
TODO
***************************************************************/

#include <nusys.h>
#include "config.h"
#include "netlib.h"
#include "packets.h"
#include "helper.h"
#include "stages.h"
#include "text.h"
#include "objects.h"
#include "datastructs.h"

static linkedList global_levelobjects;


/*==============================
    stage_game_init
    Initialize the stage
==============================*/

void stage_game_init(void)
{
    global_levelobjects = EMPTY_LINKEDLIST;
}


/*==============================
    stage_game_update
    Update stage variables every frame
==============================*/

void stage_game_update(void)
{

}


/*==============================
    stage_game_draw
    Draw the stage
==============================*/

void stage_game_draw(void)
{
    int i;
    listNode* listit;
    glistp = glist;

    // Initialize the RCP and framebuffer
    rcp_init();
    fb_clear(100, 100, 100);
    
    // Render all objects
    listit = global_levelobjects.head;
    while (listit != NULL)
    {
        GameObject* obj = (GameObject*)listit->data;
        gDPSetFillColor(glistp++, (GPACK_RGBA5551(obj->col.r, obj->col.g, obj->col.b, 1) << 16 | 
                                   GPACK_RGBA5551(obj->col.r, obj->col.g, obj->col.b, 1)));
        gDPFillRectangle(glistp++, 
            obj->pos.x - (obj->size.x/2), obj->pos.y - (obj->size.y/2),
            obj->pos.x + (obj->size.x/2), obj->pos.y + (obj->size.y/2)
        );
        gDPPipeSync(glistp++);
        listit = listit->next;
    }
    
    // Render all players
    for (i=0; i<MAXPLAYERS; i++)
    {
        if (global_players[i].connected)
        {
            GameObject* plyobj = (GameObject*)global_players[i].obj;
            gDPSetFillColor(glistp++, (GPACK_RGBA5551(plyobj->col.r, plyobj->col.g, plyobj->col.b, 1) << 16 | 
                                       GPACK_RGBA5551(plyobj->col.r, plyobj->col.g, plyobj->col.b, 1)));
            gDPFillRectangle(glistp++, 
                plyobj->pos.x - (plyobj->size.x/2), plyobj->pos.y - (plyobj->size.y/2),
                plyobj->pos.x + (plyobj->size.x/2), plyobj->pos.y + (plyobj->size.y/2)
            );
            gDPPipeSync(glistp++);
        }
    }

    // Render some text
    text_render();

    // Finish
    gDPFullSync(glistp++);
    gSPEndDisplayList(glistp++);
    nuGfxTaskStart(glist, (s32)(glistp - glist) * sizeof(Gfx), NU_GFX_UCODE_F3DEX, NU_SC_SWAPBUFFER);
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
    stage_game_createobject
    Handles an object's creation
==============================*/

void stage_game_createobject(void)
{
    GameObject* obj = objects_create();
    netlib_readdword((u32*)&obj->id);
    netlib_readfloat(&obj->pos.x);
    netlib_readfloat(&obj->pos.y);
    netlib_readfloat(&obj->dir.x);
    netlib_readfloat(&obj->dir.y);
    netlib_readfloat(&obj->size.x);
    netlib_readfloat(&obj->size.y);
    netlib_readdword((u32*)&obj->speed);
    netlib_readbyte(&obj->col.r);
    netlib_readbyte(&obj->col.g);
    netlib_readbyte(&obj->col.b);
    list_append(&global_levelobjects, obj);
}