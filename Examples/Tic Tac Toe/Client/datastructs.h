#ifndef _PRESENTATION_DATASTRUCTS_H
#define _PRESENTATION_DATASTRUCTS_H

    /*********************************
                  Macros
    *********************************/
    
    #define EMPTY_LINKEDLIST {0, NULL, NULL}
    #define HASHTABLE_SIZE 32
    

    /*********************************
                 Structs
    *********************************/
    
    /* --- Linked List --- */
    
    typedef struct listNode_t {
        void* data;
        struct listNode_t *next;
    } listNode;
    
    typedef struct {
        int size;
        listNode *head;
        listNode *tail;
    } linkedList;
    
    
    /* --- Dictionary --- */
    
    typedef struct dictNode_t {
        int key;
        void* value;
        struct dictNode_t *next;
    } dictNode;
    
    typedef struct {
        int size;
        dictNode *head;
        dictNode *tail;
    } Dictionary;
    
    
    /* --- Hashtable --- */
    
    typedef struct {
        int size;
        Dictionary table[HASHTABLE_SIZE];
    } hashTable;

    
    /*********************************
                Functions
    *********************************/
    
    // Linked list functions
    extern listNode* list_append(linkedList* list, void* data);
    extern void      list_combine(linkedList* dest, linkedList* list);
    extern listNode* list_remove(linkedList* list, void* data);
    extern void      list_destroy(linkedList* list);
    extern void      list_destroy_deep(linkedList* list);
    extern listNode* list_swapindex_withlist(linkedList* dest, int index, linkedList* list);
    extern listNode* list_node_from_index(linkedList* list, int index);
    
    // Dictionary functions
    extern dictNode* dict_append(Dictionary* dict, int key, void* data);
    extern dictNode* dict_get(Dictionary* dict, int key);
    extern void      dict_destroy(Dictionary* dict);
    extern void      dict_destroy_deep(Dictionary* dict);
    
    // Hashtable functions
    extern dictNode* htable_append(hashTable* htable, int key, void* value);
    extern dictNode* htable_get(hashTable* htable, int key);
    extern void      htable_destroy(hashTable* htable);
    extern void      htable_destroy_deep(hashTable* htable);
    
#endif