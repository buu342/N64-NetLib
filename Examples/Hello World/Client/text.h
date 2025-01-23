#ifndef HELLOSERVER_TEXT_H
#define HELLOSERVER_TEXT_H


    /*********************************
                  Macros
    *********************************/

    #define EMPTYCHARDEF {0, 0, 0, 0, NULL, 0} 
    #define BULLET1 "[ "
    #define BULLET2 "] "
    #define BULLET3 "^ "
    #define BOLD    "\\"
    #define BIGDASH " ` "
    #define BULLET1SIZE 13
    #define BULLET2SIZE 37
    #define BULLET3SIZE 56

    
    /*********************************
               Enumerations
    *********************************/
    
    typedef enum {
        ALIGN_LEFT,
        ALIGN_CENTER,
        ALIGN_RIGHT,
    } textAlign;
    

    
    /*********************************
                Structures
    *********************************/

    typedef struct {
        u8 w;
        u8 h;
        u16 offsetx;
        u16 offsety;
        u8* tex;
        u32 xpadding;
    } charDef;
    
    typedef struct {
        u8      w;
        u8      h;
        u8      spacesize;
        u8      packed;
        charDef ch[90];
    } fontDef;
    
    typedef struct {
        u16 x;
        u16 y;
        u8  r;
        u8  g;
        u8  b;
        u8  a;
        charDef* cdef;
        fontDef* fdef;
    } letterDef;

    
    /*********************************
                Functions
    *********************************/
    
    void text_initialize();
    void text_create(char* str, u16 x, u16 y);
    void text_render();
    void text_rendernumber(int num, u16 x, u16 y);
    void text_reset();
    void text_cleanup();
    void text_setfont(fontDef* font);
    void text_setalign(textAlign align);
    void text_setpos(s16 x, s16 y);
    void text_setcolor(u8 r, u8 g, u8 b, u8 a);
    s16  text_getx();
    s16  text_gety();


    /*********************************
                 Globals
    *********************************/
    
    extern fontDef font_default;
    extern fontDef font_small;
    extern fontDef font_title;
    
#endif