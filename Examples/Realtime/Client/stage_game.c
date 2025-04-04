/***************************************************************
                          stage_game.c
                               
TODO
***************************************************************/

#include <nusys.h>
#include <malloc.h>
#include "config.h"
#include "netlib.h"
#include "packets.h"
#include "helper.h"
#include "stages.h"
#include "text.h"
#include "objects.h"

#define INPUTRATE        15.0f
#define MAXPACKETSTOACK  (INPUTRATE*5*2)

static NUContData global_contdata;

static OSTime global_nextsend;

static bool global_prediction;
static bool global_reconciliation;
static bool global_interpolation;

static linkedList global_inputstoack;
static linkedList global_inputstosend;
static Vector2D   global_lastackedpos;


/*==============================
    TODO
==============================*/

void stage_game_updatetext()
{
    char buf[32];
    struct malloc_status_st st;
    malloc_memcheck(&st);
    text_cleanup();
    text_setfont(&font_small);
    text_setalign(ALIGN_LEFT);
    text_setcolor(0, 0, 0, 255);
    if (global_prediction)
        text_create("Prediction: Enabled", 32, 32);
    else
        text_create("Prediction: Disabled", 32, 32);
    if (global_reconciliation)
        text_create("Reconciliation: Enabled", 32, 32+16);
    else
        text_create("Reconciliation: Disabled", 32, 32+16);
    if (global_interpolation)
        text_create("Interpolation: Enabled", 32, 32+32);
    else
        text_create("Interpolation: Disabled", 32, 32+32);
    sprintf(buf, "Unacked inputs %d", global_inputstoack.size);
    text_create(buf, 32, 32+48);
    sprintf(buf, "Free mem %d", st.freeMemSize);
    text_create(buf, 32, 32+64);
}


/*==============================
    stage_game_init
    Initialize the stage
==============================*/

void stage_game_init(void)
{
    OSTime curtime = osGetTime();
    global_nextsend = curtime;
    global_prediction = TRUE;
    global_reconciliation = TRUE;
    global_interpolation = FALSE;
    global_inputstoack = EMPTY_LINKEDLIST;
    global_inputstosend = EMPTY_LINKEDLIST;
    global_lastackedpos = ((GameObject*)global_players[netlib_getclient()-1].obj)->pos;
    stage_game_updatetext();    
}


/*==============================
    stage_game_update
    Update stage variables every frame
==============================*/

void stage_game_update(float dt)
{
    u8 refreshtext = FALSE;
    OSTime curtime = osGetTime();
    GameObject* plyobj = global_players[netlib_getclient()-1].obj;
    InputToAck* in;
    
    // Get controller data and apply some basic deadzoning to it
    nuContDataGetEx(&global_contdata, 0);
    if (global_contdata.stick_x < MINSTICK && global_contdata.stick_x > -MINSTICK)
        global_contdata.stick_x = 0;
    else if (global_contdata.stick_x > MAXSTICK || global_contdata.stick_x < -MAXSTICK)
        global_contdata.stick_x = (global_contdata.stick_x > 0) ? MAXSTICK : -MAXSTICK;
    if (global_contdata.stick_y < MINSTICK && global_contdata.stick_y > -MINSTICK)
        global_contdata.stick_y = 0;
    else if (global_contdata.stick_y > MAXSTICK || global_contdata.stick_y < -MAXSTICK)
        global_contdata.stick_y = (global_contdata.stick_y > 0) ? MAXSTICK : -MAXSTICK;
        
    // Buffer this input so that we can dump it to the server later
    in = (InputToAck*)malloc(sizeof(InputToAck));
    in->time = curtime;
    in->contdata = global_contdata;
    in->dt = dt;
    list_append(&global_inputstosend, in);

    // Predict the player's movement before the server updates our position
    objects_applycont(plyobj, global_contdata);
    if (global_prediction)
        objects_applyphys(plyobj, dt);
    
    // Handle toggling of different clientside improvements
    if (global_contdata.trigger & R_TRIG)
    {
        global_reconciliation = !global_reconciliation;
        if (global_reconciliation && !global_prediction)
            global_prediction = TRUE;
        else if (!global_reconciliation && global_inputstoack.size > 0)
            list_destroy_deep(&global_inputstoack);
        refreshtext = TRUE;
    }
    if (global_contdata.trigger & L_TRIG)
    {
        global_prediction = !global_prediction;
        if (!global_prediction && global_reconciliation)
            global_reconciliation = FALSE;
        if (!global_reconciliation && global_inputstoack.size > 0)
            list_destroy_deep(&global_inputstoack);
        refreshtext = TRUE;
    }
    if (global_contdata.trigger & Z_TRIG)
    {
        global_interpolation = !global_interpolation;
        refreshtext = TRUE;
    }
    
    // Send the client input to the server every 15hz (if you do too high a rate, you risk flooding the USB/router)
    if (global_nextsend < curtime)
    {
        listNode* node = global_inputstosend.head;
        
        // Dump the input data that we buffered over previous frames
        netlib_start(PACKETID_CLIENTINPUT);
            netlib_writebyte((u8)global_inputstosend.size);
            while (node != NULL)
            {
                InputToAck* insend = (InputToAck*)node->data;
                netlib_writeqword((u64)insend->time);
                netlib_writefloat((f32)insend->dt);
                netlib_writebyte((u8)insend->contdata.stick_x);
                netlib_writebyte((u8)insend->contdata.stick_y);
                node = node->next;
            }
        netlib_sendtoserver();
        global_nextsend = curtime + OS_USEC_TO_CYCLES((u64)(1000000.0f*(1.0f/INPUTRATE)));
        
        // If we want to reconcile, add the buffered inputs to the reconcile list
        if (global_reconciliation)
        {
            list_combine(&global_inputstoack, &global_inputstosend);
            global_inputstosend = EMPTY_LINKEDLIST;
        }
        else // Otherwise just destroy the data to free the memory
            list_destroy_deep(&global_inputstosend);
    }
    
    // Refresh debug text if necessary
    //if (refreshtext)
        stage_game_updatetext();
}


/*==============================
    stage_game_fixedupdate
    Update stage variables every tick
==============================*/

void stage_game_fixedupdate(float dt)
{
    // Nothing
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
    listit = objects_getall()->head;
    while (listit != NULL)
    {
        GameObject* obj = (GameObject*)listit->data;
        gDPSetFillColor(glistp++, (GPACK_RGBA5551(obj->col.r, obj->col.g, obj->col.b, 1) << 16 | 
                                   GPACK_RGBA5551(obj->col.r, obj->col.g, obj->col.b, 1)));
        if (global_interpolation)
        {
            float subtick = stages_getsubtick();
            float xpos = flerp(obj->oldpos.x, obj->pos.x, subtick);
            float ypos = flerp(obj->oldpos.y, obj->pos.y, subtick);
            gDPFillRectangle(glistp++, 
                xpos - (obj->size.x/2), ypos - (obj->size.y/2),
                xpos + (obj->size.x/2), ypos + (obj->size.y/2)
            );
        }
        else
        {
            float xpos = obj->pos.x;
            float ypos = obj->pos.y;
            gDPFillRectangle(glistp++, 
                xpos - (obj->size.x/2), ypos - (obj->size.y/2),
                xpos + (obj->size.x/2), ypos + (obj->size.y/2)
            );
        }
        gDPPipeSync(glistp++);
        listit = listit->next;
    }
    
    // Render other stuff
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
    int i;
    
    // Destroy all objects
    objects_destroyall();
    
    // Cleanup player structs
    for (i=0; i<MAXPLAYERS; i++)
    {
        global_players[i].connected = FALSE;
        global_players[i].obj = NULL;
    }
    
    // Other cleanup
    text_cleanup();
    list_destroy_deep(&global_inputstoack);
    list_destroy_deep(&global_inputstosend);
}


/*==============================
    TODO
==============================*/

void stage_game_ackinput(OSTime time, Vector2D pos)
{
    if (global_reconciliation)
    {
        GameObject* plyobj = global_players[netlib_getclient()-1].obj;
        listNode* node = global_inputstoack.head;
        
        // Go through the packets, and remove any that are outdated
        // Since this list is FIFO, as soon as we find a packet with a later time, we can stop
        while (node != NULL)
        {
            InputToAck* in = (InputToAck*)node->data;
            if (in->time <= time)
            {
                free(list_remove(&global_inputstoack, in));
                free(in);
            }
            else
                break;
            node = node->next;
        }
        
        // Set our pos to the acknowledged position
        global_lastackedpos = pos;
        plyobj->pos = global_lastackedpos;
        
        // Now go through all packets that are yet to be acknowledged and reapply them to reconcile the position
        node = global_inputstoack.head;
        while (node != NULL)
        {
            InputToAck* in = (InputToAck*)node->data;
            objects_applycont(plyobj, in->contdata);
            objects_applyphys(plyobj, in->dt);
            node = node->next;
        }
    }
}