#include "extension.h"

using namespace SourceHook;

SH_DECL_HOOK1_void(IServerGameDLL, SaveGlobalState, SH_NOATTRIB, 0, CSaveRestoreData *);
SH_DECL_HOOK1_void(IServerGameDLL, RestoreGlobalState, SH_NOATTRIB, 0, CSaveRestoreData *);

void Hook_SaveGlobalState(CSaveRestoreData *saveRestoreData)
{
    META_CONPRINTF("SAVING\n");
    RETURN_META(MRES_HANDLED);
}

void Hook_RestoreGlobalState(CSaveRestoreData *saveRestoreData)
{
    META_CONPRINTF("RESTORING\n");
    RETURN_META(MRES_HANDLED);
}

void AddFMODStateHooks()
{
   SH_ADD_HOOK(IServerGameDLL, SaveGlobalState, gamedll, SH_STATIC(Hook_SaveGlobalState), false);
   SH_ADD_HOOK(IServerGameDLL, RestoreGlobalState, gamedll, SH_STATIC(Hook_RestoreGlobalState), false);
}
 
void RemoveFMODStateHooks()
{
   SH_REMOVE_HOOK(IServerGameDLL, SaveGlobalState, gamedll, SH_STATIC(Hook_SaveGlobalState), false);
   SH_REMOVE_HOOK(IServerGameDLL, RestoreGlobalState, gamedll, SH_STATIC(Hook_RestoreGlobalState), false);
}