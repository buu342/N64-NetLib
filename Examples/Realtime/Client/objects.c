#include <ultra64.h>
#include "objects.h"


/*********************************
             Globals
*********************************/

Player global_players[MAXPLAYERS];


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
}


/*==============================
    objects_connectplayer
    TODO
==============================*/

void objects_connectplayer(u8 num, GameObject* obj)
{
    global_players[num].connected = TRUE;
    global_players[num].obj = obj;
}


/*==============================
    objects_disconnectplayer
    TODO
==============================*/

void objects_disconnectplayer(u8 num)
{
    global_players[num].connected = FALSE;
    objects_destroy(global_players[num].obj);
}


/*==============================
    objects_create
    TODO
==============================*/

GameObject* objects_create()
{
    return (GameObject*)malloc(sizeof(GameObject));
}


/*==============================
    objects_destroy
    TODO
==============================*/

void objects_destroy(GameObject* obj)
{
    free(obj);
}