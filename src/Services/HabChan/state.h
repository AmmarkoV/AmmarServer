#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED


#include "../../AmmServerlib/AmmServerlib.h"
#include "../../AmmServerlib/hashmap/hashmap.h"

#include "state.h"


extern struct AmmServer_Instance  * default_server;
extern struct AmmServer_Instance  * admin_server;
extern struct AmmServer_RequestOverride_Context GET_override;


extern struct hashMap * boardHashMap ;

int loadBoards();

int unloadBoards();

#endif // STATE_H_INCLUDED
