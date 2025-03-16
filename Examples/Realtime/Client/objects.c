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
    objects_draw
    TODO
==============================*/

void objects_draw()
{
    listNode* listit = global_levelobjects.head;
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