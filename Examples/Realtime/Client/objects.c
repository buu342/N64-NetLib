/***************************************************************
                           objects.c
                               
Handles the object system used in this game
***************************************************************/

#include <nusys.h>
#include <malloc.h>
#include "objects.h"
#include "config.h"
#include "helper.h"


/*********************************
             Globals
*********************************/

Player global_players[MAXPLAYERS];
static linkedList global_levelobjects;


/*==============================
    objects_initsystem
    Initializes the object system.
    Call once at the start of the game.
==============================*/

void objects_initsystem()
{
    int i;
    for (i=0; i<MAXPLAYERS; i++)
    {
        global_players[i].connected = FALSE;
        global_players[i].obj = NULL;
    }
    global_levelobjects = EMPTY_LINKEDLIST;
}


/*==============================
    objects_create
    Creates a new game object.
    @return The newly created object
==============================*/

GameObject* objects_create()
{
    GameObject* obj = (GameObject*)malloc(sizeof(GameObject));
    list_append(&global_levelobjects, obj);
    return obj;
}


/*==============================
    objects_findbyid
    Finds an object by searching for its unique ID
    @param  The ID of the object to find
    @return The found object, or NULL
==============================*/

GameObject* objects_findbyid(u32 id)
{
    listNode* listit = global_levelobjects.head;
    while (listit != NULL)
    {
        GameObject* obj = (GameObject*)listit->data;
        if (obj->id == id)
            return obj;
        listit = listit->next;
    }
    return NULL;
}


/*==============================
    objects_getall
    Gets the list of all the objects in the game
    @return A pointer to the list of objects in the game
==============================*/

linkedList* objects_getall()
{
    return &global_levelobjects;
}


/*==============================
    objects_destroy
    Destroys an object and frees its memory
    @param The object to destroy
==============================*/

void objects_destroy(GameObject* obj)
{
    free(list_remove(&global_levelobjects, obj));
    free(obj);
}


/*==============================
    objects_destroyall
    Destroys all objects in the game and frees their memory.
==============================*/

void objects_destroyall()
{
    listNode* listit = global_levelobjects.head;
    while (listit != NULL)
    {
        listit = listit->next;
        objects_destroy((GameObject*)listit->data);
    }
    list_destroy(&global_levelobjects);
}


/*==============================
    objects_connectplayer
    Connects the player to the game.
    @param The number of the player to connected
    @param The object to use for the player
==============================*/

void objects_connectplayer(u8 num, GameObject* obj)
{
    global_players[num-1].connected = TRUE;
    global_players[num-1].obj = obj;
}


/*==============================
    objects_disconnectplayer
    Disconnects a player from the game.
    @param The number of the player to disconnect
==============================*/

void objects_disconnectplayer(u8 num)
{
    global_players[num-1].connected = FALSE;
    objects_destroy(global_players[num-1].obj);
    global_players[num-1].obj = NULL;
}


/*==============================
    objects_applycont
    Applies the controller data to a specific object, clientside.
    @param The object to manipulate
    @param The controller data to use
==============================*/

void objects_applycont(GameObject* obj, NUContData contdata)
{
    const float MAXSPEED = 50;
    if (contdata.stick_x < MINSTICK && contdata.stick_x > -MINSTICK)
        contdata.stick_x = 0;
    else if (contdata.stick_x > MAXSTICK || contdata.stick_x < -MAXSTICK)
        contdata.stick_x = (contdata.stick_x > 0) ? MAXSTICK : -MAXSTICK;
    if (contdata.stick_y < MINSTICK && contdata.stick_y > -MINSTICK)
        contdata.stick_y = 0;
    else if (contdata.stick_y > MAXSTICK || contdata.stick_y < -MAXSTICK)
        contdata.stick_y = (contdata.stick_y > 0) ? MAXSTICK : -MAXSTICK;
    obj->cl_trans.dir = vector_normalize((Vector2D){contdata.stick_x, -contdata.stick_y});
    obj->cl_trans.speed = (sqrtf(contdata.stick_x*contdata.stick_x + contdata.stick_y*contdata.stick_y)/MAXSTICK)*MAXSPEED;
}


/*==============================
    objects_applyphys
    Applies physics to a specific object, clientside.
    @param The object to manipulate
    @param The timestep taken
==============================*/

void objects_applyphys(GameObject* obj, float dt)
{
    int i;
    Vector2D target_offset = (Vector2D){obj->cl_trans.dir.x*obj->cl_trans.speed*dt, obj->cl_trans.dir.y*obj->cl_trans.speed*dt};
    if (obj->cl_trans.pos.x + obj->cl_trans.size.x/2 + target_offset.x > 320)
    {
        target_offset.x = target_offset.x - 2*((obj->cl_trans.pos.x + obj->cl_trans.size.x/2 + target_offset.x) - 320);
        if (obj->bounce)
            obj->cl_trans.dir.x = -obj->cl_trans.dir.x;
    }
    if (obj->cl_trans.pos.x - obj->cl_trans.size.x/2 + target_offset.x < 0)
    {
        target_offset.x = target_offset.x - 2*((obj->cl_trans.pos.x - obj->cl_trans.size.x/2 + target_offset.x) - 0);
        if (obj->bounce)
            obj->cl_trans.dir.x = -obj->cl_trans.dir.x;
    }
    if (obj->cl_trans.pos.y + obj->cl_trans.size.y/2 + target_offset.y > 240)
    {
        target_offset.y = target_offset.y - 2*((obj->cl_trans.pos.y + obj->cl_trans.size.y/2 + target_offset.y) - 240);
        if (obj->bounce)
            obj->cl_trans.dir.y = -obj->cl_trans.dir.y;
    }
    if (obj->cl_trans.pos.y - obj->cl_trans.size.y/2 + target_offset.y < 0)
    {
        target_offset.y =target_offset.y - 2*((obj->cl_trans.pos.y - obj->cl_trans.size.y/2 + target_offset.y) - 0);
        if (obj->bounce)
            obj->cl_trans.dir.y = -obj->cl_trans.dir.y;
    }
    obj->cl_trans.pos.x += target_offset.x;
    obj->cl_trans.pos.y += target_offset.y;
}


/*==============================
    objects_pusholdtransforms
    Pushes all the old transforms back one to make room for a new transform
    @param The object to push the transforms of
==============================*/

void objects_pusholdtransforms(GameObject* obj)
{
    int i;
    if (obj == NULL)
        return;
    for (i=TICKSTOKEEP-1; i>0; i--)
        obj->old_trans[i] = obj->old_trans[i-1];
    obj->old_trans[0] = obj->sv_trans;
}


/*==============================
    objects_synctransforms
    Syncs the transforms between the client and server values, and performs
    calculations to fill in any missing ticks in the timeline.
    @param The object to sync the transforms of
==============================*/

void objects_synctransforms(GameObject* obj)
{
    int i;
    u8 propagate = FALSE;
    if (obj == NULL)
        return;
    obj->cl_trans = obj->sv_trans;
    
    // Since the server only sends object updates when something changes, we have a bit of a problem
    // because it can be many ticks without an object being updated. This means that if we try to interpolate
    // based on these timestamps, it will be wrong. So we need to fill in the gaps if the time delta is too big.
    // I'm not doing this in the most accurate way, just the fastest way.
    for (i=-1; i<TICKSTOKEEP-1; i++)
    {
        Transform* older = &obj->old_trans[i+1];
        Transform* newer = ((i != -1) ? (&obj->old_trans[i]) : (&obj->sv_trans));
        OSTime delta = newer->timestamp - older->timestamp;
        if (!propagate && delta > OS_USEC_TO_CYCLES(SEC_TO_USEC(DELTATIME*1.5f)))
            propagate = TRUE;
        if (propagate)
        {
            *older = *newer;
            older->timestamp = newer->timestamp - OS_USEC_TO_CYCLES(SEC_TO_USEC(DELTATIME));
        }
    }
}