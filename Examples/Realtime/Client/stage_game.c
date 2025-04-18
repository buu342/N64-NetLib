/***************************************************************
                          stage_game.c
                               
The main room where all the players and NPCs interact
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


/*********************************
              Macros
*********************************/

#define INPUTRATE        15.0f
#define MAXPACKETSTOACK  100


/*********************************
             Globals
*********************************/

// Controller related
static NUContData global_contdata;
static OSTime global_nextsend;

// Clientside improvements
static bool global_prediction;
static bool global_reconciliation;
static bool global_interpolation;

// Synchronization related
static linkedList global_inputstoack;
static linkedList global_inputstosend;


/*==============================
    stage_game_updatetext
    Refreshes the text on the screen
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
    global_prediction = FALSE;
    global_reconciliation = FALSE;
    global_interpolation = FALSE;
    global_inputstoack = EMPTY_LINKEDLIST;
    global_inputstosend = EMPTY_LINKEDLIST;
    stage_game_updatetext();    
}


/*==============================
    stage_game_update
    Update stage variables every frame
    @param The deltatime (in seconds)
==============================*/

void stage_game_update(float dt)
{
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
    if (global_prediction)
    {
        objects_applycont(plyobj, global_contdata);
        objects_applyphys(plyobj, dt);
    }
    
    // Handle toggling of different clientside improvements
    if (global_contdata.trigger & R_TRIG)
    {
        global_reconciliation = !global_reconciliation;
        if (global_reconciliation && !global_prediction)
            global_prediction = TRUE;
        else if (!global_reconciliation && global_inputstoack.size > 0)
            list_destroy_deep(&global_inputstoack);
    }
    if (global_contdata.trigger & L_TRIG)
    {
        global_prediction = !global_prediction;
        if (!global_prediction && global_reconciliation)
            global_reconciliation = FALSE;
        if (!global_reconciliation && global_inputstoack.size > 0)
            list_destroy_deep(&global_inputstoack);
    }
    if (global_contdata.trigger & Z_TRIG)
        global_interpolation = !global_interpolation;
    
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
            while (global_inputstoack.size > MAXPACKETSTOACK)
            {
                InputToAck* inclean = global_inputstoack.head->data;
                free(list_remove(&global_inputstoack, inclean));
                free(inclean);
            }
            global_inputstosend = EMPTY_LINKEDLIST;
        }
        else // Otherwise just destroy the data to free the memory
            list_destroy_deep(&global_inputstosend);
    }
    
    // Refresh debug text
    stage_game_updatetext();
}


/*==============================
    stage_game_draw
    Draw the stage
==============================*/

void stage_game_draw(void)
{
    char buff[128];
    int i;
    OSTime curtime = osGetTime();
    listNode* listit;
    glistp = glist;

    // Initialize the RCP and framebuffer
    rcp_init();
    fb_clear(100, 100, 100);
    
    // Render all objects
    listit = objects_getall()->head;
    while (listit != NULL)
    {
        float xpos, ypos;
        GameObject* obj = (GameObject*)listit->data;
        gDPSetFillColor(glistp++, (GPACK_RGBA5551(obj->cl_trans.col.r, obj->cl_trans.col.g, obj->cl_trans.col.b, 1) << 16 | 
                                   GPACK_RGBA5551(obj->cl_trans.col.r, obj->cl_trans.col.g, obj->cl_trans.col.b, 1)));
        if (global_interpolation && obj != global_players[netlib_getclient()-1].obj) // Interpolate if not the client (since there's no need to)
        {
            float timediff;
        	OSTime time = curtime - OS_USEC_TO_CYCLES(SEC_TO_USEC(VIEWLAG));
            Transform* before;
            Transform* after;
	
            // Find two points that surround our current view time
            if (obj->old_trans[0].timestamp <= time)
            {
                before = &obj->old_trans[0];
                after = &obj->sv_trans;
            }
            else
            {
                int i;
                for (i=0; i<TICKSTOKEEP-1; i++)
                {
                    before = &obj->old_trans[i+1];
                    after = &obj->old_trans[i];
                    if (before->timestamp <= time)
                        break;
                }
            }
            timediff = ((double)(time - before->timestamp))/((double)(after->timestamp - before->timestamp));
            timediff = CLAMP(timediff, 0.0f, 1.0f);
            
            // Set the clientside position to the interpolated value
            obj->cl_trans.pos.x = flerp(before->pos.x, after->pos.x, timediff);
            obj->cl_trans.pos.y = flerp(before->pos.y, after->pos.y, timediff);
        }
        
        // Draw the object at the clientside position
        xpos = obj->cl_trans.pos.x;
        ypos = obj->cl_trans.pos.y;
        gDPFillRectangle(glistp++, 
            xpos - (obj->cl_trans.size.x/2), ypos - (obj->cl_trans.size.y/2),
            xpos + (obj->cl_trans.size.x/2), ypos + (obj->cl_trans.size.y/2)
        );
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
    stage_game_ackinput
    Acknowledge the input and reconcile our position
    @param The last acknowledged input time
    @param Whether to reconcile or not.
           Since the server only sends position updates when it changes, we
           should only re-apply the inputs if our position was changed this packet.
==============================*/

void stage_game_ackinput(OSTime time, u8 reconcile)
{
    if (global_reconciliation)
    {
        GameObject* plyobj = global_players[netlib_getclient()-1].obj;
        listNode* node = global_inputstoack.head;
        
        // Go through the packets, remove any that are acknowledged, and reapply the rest to reconcile the position
        while (node != NULL)
        {
            InputToAck* in = (InputToAck*)node->data;
            node = node->next;
            if (in->time <= time)
            {                
                free(list_remove(&global_inputstoack, in));
                free(in);
            }
            else if (reconcile)
            {
                objects_applycont(plyobj, in->contdata);
                objects_applyphys(plyobj, in->dt);
            }
        }
    }
}