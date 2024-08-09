#ifndef _N64_NETLIB_H
#define _N64_NETLIB_H
    
    void netlib_initialize();
    
    
    /*==============================================================
                        N64 -> Network Functions
    ==============================================================*/
    
    void netlib_start();
    
    void netlib_writebyte();
    
    void netlib_writeword();
    
    void netlib_writedword();
    
    void netlib_writebytes();
    
    void netlib_broadcast();
    
    void netlib_sendtoserver();
    
    void netlib_sendtoclient();
    
    
    /*==============================================================
                        Network -> N64 Functions
    ==============================================================*/
    
    void netlib_register();
    
    void netlib_poll();
    
    void netlib_readbyte();
    
    void netlib_readword();
    
    void netlib_readdword();
    
    void netlib_readbytes();
    
#endif