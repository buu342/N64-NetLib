/***************************************************************
                           objects.c
                               
TODO
***************************************************************/

#include <nusys.h>
#include <malloc.h>
#include "objects.h"
#include "config.h"


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
    list_remove(&global_levelobjects, obj);
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

void objects_applycont(GameObject* obj, NUContData contdata)
{
    const float MAXSPEED = 5;
    if (contdata.stick_x < MINSTICK && contdata.stick_x > -MINSTICK)
        contdata.stick_x = 0;
    else if (contdata.stick_x > MAXSTICK || contdata.stick_x < -MAXSTICK)
        contdata.stick_x = (contdata.stick_x > 0) ? MAXSTICK : -MAXSTICK;
    if (contdata.stick_y < MINSTICK && contdata.stick_y > -MINSTICK)
        contdata.stick_y = 0;
    else if (contdata.stick_y > MAXSTICK || contdata.stick_y < -MAXSTICK)
        contdata.stick_y = (contdata.stick_y > 0) ? MAXSTICK : -MAXSTICK;
    obj->dir = vector_normalize((Vector2D){contdata.stick_x, -contdata.stick_y});
    obj->speed = ((sqrtf(contdata.stick_x*contdata.stick_x + contdata.stick_y*contdata.stick_y)/MAXSTICK)*MAXSPEED)*100;
}

void objects_applyphys(GameObject* obj, float dt)
{
    Vector2D target_offset = (Vector2D){obj->dir.x*obj->speed*dt, obj->dir.y*obj->speed*dt};
    if (obj->pos.x + obj->size.x/2 + target_offset.x > 320)
        target_offset.x = target_offset.x - 2*((obj->pos.x + obj->size.x/2 + target_offset.x) - 320);
    if (obj->pos.x - obj->size.x/2 + target_offset.x < 0)
        target_offset.x = target_offset.x - 2*((obj->pos.x - obj->size.x/2 + target_offset.x) - 0);
    if (obj->pos.y + obj->size.y/2 + target_offset.y > 240)
        target_offset.y = target_offset.y - 2*((obj->pos.y + obj->size.y/2 + target_offset.y) - 240);
    if (obj->pos.y - obj->size.y/2 + target_offset.y < 0) 
        target_offset.y =target_offset.y - 2*((obj->pos.y - obj->size.y/2 + target_offset.y) - 0);
    obj->pos.x += target_offset.x;
    obj->pos.y += target_offset.y;
}