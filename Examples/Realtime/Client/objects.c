/***************************************************************
                           objects.c
                               
TODO
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
    TODO
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
    TODO
==============================*/

GameObject* objects_create()
{
    GameObject* obj = (GameObject*)malloc(sizeof(GameObject));
    list_append(&global_levelobjects, obj);
    return obj;
}


/*==============================
    objects_findbyid
    TODO
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
    TODO
==============================*/

linkedList* objects_getall()
{
    return &global_levelobjects;
}


/*==============================
    objects_destroy
    TODO
==============================*/

void objects_destroy(GameObject* obj)
{
    free(list_remove(&global_levelobjects, obj));
    free(obj);
}


/*==============================
    objects_destroyall
    TODO
==============================*/

void objects_destroyall()
{
    listNode* listit = global_levelobjects.head;
    while (listit != NULL)
    {
        objects_destroy((GameObject*)listit->data);
        listit = listit->next;
    }
    list_destroy(&global_levelobjects);
}


/*==============================
    objects_connectplayer
    TODO
==============================*/

void objects_connectplayer(u8 num, GameObject* obj)
{
    global_players[num-1].connected = TRUE;
    global_players[num-1].obj = obj;
}


/*==============================
    objects_disconnectplayer
    TODO
==============================*/

void objects_disconnectplayer(u8 num)
{
    global_players[num-1].connected = FALSE;
    objects_destroy(global_players[num-1].obj);
    global_players[num-1].obj = NULL;
}


/*==============================
    TODO
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
    TODO
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
    TODO
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
    TODO
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