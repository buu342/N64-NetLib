/***************************************************************
                             text.c
                             
Renders text using optimized texture loads. Not every character
is available for every font, since I didn't need them for the 
presentation.
If using the default font, the following characters will perform
special effects:
[ = Bullet point level 1
] = Bullet point level 2
^ = Bullet point level 3
\ = Toggle bold text
***************************************************************/

#include <nusys.h>
#include <malloc.h>
#include <string.h>
#include "config.h"
#include "datastructs.h"
#include "text.h"
#include "debug.h"
#include "assets/font_title.h"
#include "assets/font_default.h"
#include "assets/font_default_bold.h"
#include "assets/font_small.h"


/*********************************
              Macros
*********************************/

#define SPACESIZE 8
#define MAXMIMUM_FONTTEXTURES 32


/*********************************
             Globals
*********************************/

// Text settings
static fontDef*  textrender_font;
static textAlign textrender_align;
static s16       textrender_startx;
static s16       textrender_starty;
static u8        textrender_r;
static u8        textrender_g;
static u8        textrender_b;
static u8        textrender_a;

// Text rendering order globals
static int        textrender_fontkey;
static Dictionary textrender_addressmap;
static linkedList textrender_loadlist[MAXMIMUM_FONTTEXTURES];


/*==============================
    text_initialize
    Initializes the text globals
==============================*/

void text_initialize()
{
    textrender_fontkey = 0;
    memset(&textrender_addressmap, 0, sizeof(Dictionary));
    memset(&textrender_loadlist, 0, sizeof(linkedList)*MAXMIMUM_FONTTEXTURES);
    text_reset();
}


/*==============================
    text_reset
    Sets the text settings ot their
    default values
==============================*/

void text_reset()
{
    text_setfont(&font_default);
    text_setalign(ALIGN_LEFT);
    text_setpos(0, 0);
    text_setcolor(0, 0, 0, 255);
}


/*==============================
    text_create
    Creates text on the screen
    @param The string to draw
    @param The screen X position to draw the text
    @param The screen Y position to draw the text
==============================*/

void text_create(char* str, u16 x, u16 y)
{
    int i;
    int charcount = 0;
    int hsize = 0, xreal;
    u8 bold = FALSE;
    
    // Iterate through all characters in the string to calculate the text size and create table keys
    for (i=0; str[i] != '\0'; i++) 
    {
        int texaddr;
        charDef* cdef;
        dictNode* node;
        
        // Handle special characters
        switch (str[i])
        {
            case ' ':  hsize += textrender_font->spacesize;   continue; // Skip spaces
            case '[':  hsize += textrender_font->spacesize;   break;    // Bullet point 1
            case ']':  hsize += textrender_font->spacesize*3; break;    // Bullet point 2
            case '^':  hsize += textrender_font->spacesize*7; break;    // Bullet point 3
            case '\\': 
                bold = !bold;
                if (bold)
                    textrender_font = &font_default_bold;
                else
                    textrender_font = &font_default;
                continue;
        }
            
        // Check if the current font's texture has been added to the dictionary (that maps font definition addresses to a key)
        cdef = &textrender_font->ch[str[i] - '!'];
        if (cdef->tex == NULL)
        {
            debug_printf("Warning, unsupported character '%c'\n", str[i]);
            continue;
        }
        
        texaddr = (int)cdef->tex;
        hsize += cdef->w + cdef->xpadding;
        node = dict_get(&textrender_addressmap, texaddr);
        charcount++;
        
        // If it hasn't been added, then do so
        if (node == NULL)
        {
            node = dict_append(&textrender_addressmap, texaddr, (int*)textrender_fontkey);
            textrender_fontkey++;
        }
    }
    
    // Calculate horizontal alignment
    switch (textrender_align)
    {
        case ALIGN_LEFT:   xreal = x;         break;
        case ALIGN_CENTER: xreal = x-hsize/2; break;
        case ALIGN_RIGHT:  xreal = x-hsize;   break;
    }
    
    // Iterate through all characters in the string again, allocating memory for the letters to render
    bold = FALSE;
    for (i=0; str[i] != '\0'; i++)
    {
        int texaddr, texkey;
        charDef* cdef;
        dictNode* node;
        letterDef* letter;
        
        // Handle special characters
        switch (str[i])
        {
            case ' ':  xreal += textrender_font->spacesize;   continue; // Skip spaces
            case '[':  xreal += textrender_font->spacesize;   break;    // Bullet point 1
            case ']':  xreal += textrender_font->spacesize*3; break;    // Bullet point 2
            case '^':  xreal += textrender_font->spacesize*7; break;    // Bullet point 3
            case '\\': 
                bold = !bold;
                if (bold)
                    textrender_font = &font_default_bold;
                else
                    textrender_font = &font_default;
                continue;
        }
            
        // Get this character's info
        cdef = &textrender_font->ch[str[i]-'!'];
        if (cdef->tex == NULL)
        {
            debug_printf("Warning, unsupported character '%c'\n", str[i]);
            continue;
        }
        texaddr = (int)cdef->tex;
        node = dict_get(&textrender_addressmap, texaddr);
        
        // Get our "real" key for the linked list array
        texkey = (int)node->value;
        
        // Allocate memory for a letter, and initialize it
        letter = (letterDef*) malloc(sizeof(letterDef));
        debug_assert(letter != NULL);
        letter->x = xreal;
        if (textrender_font->packed)
            letter->y = y + (cdef->offsety%(textrender_font->h/2));
        else
            letter->y = y + cdef->offsety;
        letter->r = textrender_r;
        letter->g = textrender_g;
        letter->b = textrender_b;
        letter->a = textrender_a;
        letter->cdef = cdef;
        letter->fdef = textrender_font;
        
        // Add this letter to our hashtable
        list_append(&textrender_loadlist[texkey], letter);
        
        // Calculate the next letter's position
        xreal += cdef->w + cdef->xpadding;
    }
}


/*==============================
    text_render
    Actually renders the text
==============================*/

void text_render()
{
    int i;
    
    // Initialize the text drawing settings
    gDPSetCycleType(glistp++, G_CYC_1CYCLE);
    gDPSetTexturePersp(glistp++, G_TP_NONE);
    gSPClearGeometryMode(glistp++, G_ZBUFFER);
    gDPSetRenderMode(glistp++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF);
    gDPSetCombineMode(glistp++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetTextureFilter(glistp++, G_TF_POINT);
    
    // Iterate through the hash table
    for (i=0; i<MAXMIMUM_FONTTEXTURES; i++)
    {
        listNode* node = textrender_loadlist[i].head;
        
        // Skip this table if its empty
        if (textrender_loadlist[i].size == 0)
            continue;
            
        // Load the texture
        gDPLoadTextureBlock(glistp++, ((letterDef*)(node->data))->cdef->tex, G_IM_FMT_IA, G_IM_SIZ_8b, textrender_font->w, textrender_font->h, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gDPPipeSync(glistp++);
        
        // Render all the letters that use this texture
        for (; node != NULL; node = node->next)
        {
            letterDef* letter = (letterDef*)node->data;
            charDef* cdef = letter->cdef;
            gDPSetPrimColor(glistp++, 0, 0, letter->r, letter->g, letter->b, letter->a);
            gSPScisTextureRectangle(glistp++, 
                (textrender_startx + letter->x) << 2, (textrender_starty + letter->y) << 2, 
                (textrender_startx + letter->x + cdef->w) << 2, (textrender_starty + letter->y + cdef->h) << 2, 
                G_TX_RENDERTILE, 
                cdef->offsetx << 5, cdef->offsety << 5, 
                1 << 10, 1 << 10
            );
        }
    }
}


/*==============================
    text_rendernumber
    Render a number on the screen
    for a single frame
    @param The number to render
    @param The screen X position to draw the number
    @param The screen Y position to draw the number
==============================*/

void text_rendernumber(int num, u16 x, u16 y)
{
    int i;
    char str[32];
    int xoffset = 0;
    
    // Convert the number to a string
    sprintf(str, "%d", num);
    
    // Initialize the text drawing settings
    gDPSetCycleType(glistp++, G_CYC_1CYCLE);
    gDPSetTexturePersp(glistp++, G_TP_NONE);
    gSPClearGeometryMode(glistp++, G_ZBUFFER);
    gDPSetRenderMode(glistp++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF);
    gDPSetCombineMode(glistp++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetTextureFilter(glistp++, G_TF_POINT);
    gDPLoadTextureBlock(glistp++, font_default_tex7, G_IM_FMT_IA, G_IM_SIZ_8b, textrender_font->w, textrender_font->h, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    gDPSetPrimColor(glistp++, 0, 0, 0, 0, 0, 255);
    gDPPipeSync(glistp++);
    
    // Draw each character in the string
    for (i=0; str[i] != '\0'; i++)
    {
        charDef* cdef = &font_default.ch[str[i]-'!'];
        gSPScisTextureRectangle(glistp++, 
            (x + xoffset) << 2, y << 2, 
            (x + xoffset + cdef->w) << 2, (y + cdef->h) << 2, 
            G_TX_RENDERTILE, 
            cdef->offsetx << 5, cdef->offsety << 5, 
            1 << 10, 1 << 10
        );
        xoffset += cdef->w;
    }
}


/*==============================
    text_cleanup
    Frees all the memory used by the 
    text hashtable
==============================*/

void text_cleanup()
{
    int i;
    nuGfxTaskAllEndWait();
    for (i=0; i<MAXMIMUM_FONTTEXTURES; i++)
        if (textrender_loadlist[i].size != 0)
            list_destroy_deep(&textrender_loadlist[i]);
}


/*==============================
    text_setfont
    Change the global text font setting
    @param The font definition to use
==============================*/

void text_setfont(fontDef* font)
{
    textrender_font = font;
}


/*==============================
    text_setalign
    Change the global text alignment setting
    @param The alignment to use
==============================*/

void text_setalign(textAlign align)
{
    textrender_align = align;
}


/*==============================
    text_setalign
    Change the global text starting coordinate
    setting
    @param The starting X screen coordinate
    @param The starting Y screen coordinate
==============================*/

void text_setpos(s16 x, s16 y)
{
    textrender_startx = x; 
    textrender_starty = y; 
}


/*==============================
    text_setcolor
    Change the global text color setting
    @param The red amount
    @param The green amount
    @param The blue amount
    @param The alpha
==============================*/

void text_setcolor(u8 r, u8 g, u8 b, u8 a)
{
    textrender_r = r;
    textrender_g = g;
    textrender_b = b;
    textrender_a = a;
}


/*==============================
    text_getx
    Returns the current global starting X
    coordinate for text rendering
    @returns The starting X coordinate
==============================*/

s16 text_getx()
{
    return textrender_startx;
}


/*==============================
    text_gety
    Returns the current global starting Y
    coordinate for text rendering
    @returns The starting Y coordinate
==============================*/

s16 text_gety()
{
    return textrender_starty;
}