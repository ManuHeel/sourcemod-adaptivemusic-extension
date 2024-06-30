#include "extension.h"

using namespace SourceHook;

/**
 * Save the current state of bank, event and global parameters to a .kv file with the same name as the .sav file
 */
void SaveMusicState(const char* musicStateSaveName) {
    META_CONPRINTF("AdaptiveMusic Plugin - Saving the Adaptive Music state\n");
    /*
    // Find the adaptive music file
    char szFullName[512];
    sprintf(szFullName, sizeof(szFullName), "maps/%s_adaptivemusic.txt", STRING(gpGlobals->mapname));
    auto* keyValue = new KeyValues("adaptive_music");
    if (keyValue->LoadFromFile(filesystem, szFullName, "MOD")) {
        Msg("FMOD Adaptive Music - Loading adaptive music data from '%s'\n", szFullName);
        KeyValues* keyValueSubset = keyValue->GetFirstSubKey();
        while (keyValueSubset) {
            ParseKeyValue(keyValueSubset);
            keyValueSubset = keyValueSubset->GetNextKey();
        }
    }
    */
}

/**
 * Restore the current state of bank, event and global parameters from a .kv file with the same name as the .sav file
 */
void RestoreMusicState(const char* musicStateSaveName) {
    META_CONPRINTF("AdaptiveMusic Plugin - Restoring the Adaptive Music state\n");
}

/**
 * Helper function to replace the .sav file to a .musicstate.kv file
 */
const char* replaceSavWithMusicState(const char* original) {
    // The prefixes to be removed and suffixes to be replaced
    const char* prefix1 = "save\\";
    const char* prefix2 = "save/";
    const char* oldSuffix = ".sav";
    const char* newSuffix = ".musicstate.kv";
    // Find the length of the original string and the suffixes
    size_t originalLength = strlen(original);
    size_t prefix1Length = strlen(prefix1);
    size_t prefix2Length = strlen(prefix2);
    size_t oldSuffixLength = strlen(oldSuffix);
    size_t newSuffixLength = strlen(newSuffix);
    // Pointer to the start of the actual data after prefix removal
    const char* dataStart = original;
    // Check and remove the prefix if present
    if (strncmp(original, prefix1, prefix1Length) == 0) {
        dataStart = original + prefix1Length;
    } else if (strncmp(original, prefix2, prefix2Length) == 0) {
        dataStart = original + prefix2Length;
    }
    // Recalculate the length of the string without the prefix
    size_t dataLength = strlen(dataStart);
    // Check if the string ends with ".sav"
    if (dataLength < oldSuffixLength || strcmp(dataStart + dataLength - oldSuffixLength, oldSuffix) != 0) {
        // If not, return a copy of the string without the prefix
        char* result = new char[dataLength + 1];
        strcpy(result, dataStart);
        return result;
    }
    // Calculate the length of the new string
    size_t newLength = dataLength - oldSuffixLength + newSuffixLength;
    // Allocate memory for the new string
    char* result = new char[newLength + 1]; // +1 for null terminator
    // Copy the string up to (but not including) the ".sav" part
    strncpy(result, dataStart, dataLength - oldSuffixLength);
    // Append the new suffix
    strcpy(result + dataLength - oldSuffixLength, newSuffix);
    // Return the result as a const char*
    return result;
}

SH_DECL_HOOK1_void(IServerGameDLL, SaveGlobalState, SH_NOATTRIB, 0, CSaveRestoreData *);
SH_DECL_HOOK1_void(IServerGameDLL, RestoreGlobalState, SH_NOATTRIB, 0, CSaveRestoreData *);
// Testing
SH_DECL_HOOK1_void(IServerGameDLL, PreSave, SH_NOATTRIB, 0, CSaveRestoreData *);
SH_DECL_HOOK1_void(IServerGameDLL, Save, SH_NOATTRIB, 0, CSaveRestoreData *);
SH_DECL_HOOK2_void(IServerGameDLL, Restore, SH_NOATTRIB, 0, CSaveRestoreData *, bool);
SH_DECL_HOOK2_void(IServerGameDLL, PreSaveGameLoaded, SH_NOATTRIB, 0, char const *, bool);

void Hook_SaveGlobalState(CSaveRestoreData *saveRestoreData)
{
    META_CONPRINTF("SAVING GLOBAL STATE\n");
    const char *saveName = engine->GetSaveFileName();
    const char *musicStateSaveName = replaceSavWithMusicState(saveName);
    SaveMusicState(musicStateSaveName);
    RETURN_META(MRES_HANDLED);
}

void Hook_RestoreGlobalState(CSaveRestoreData *saveRestoreData)
{
    META_CONPRINTF("RESTORING GLOBAL STATE\n");
    const char * saveName = engine->GetMostRecentlyLoadedFileName();
    const char *musicStateSaveName = replaceSavWithMusicState(saveName);
    RestoreMusicState(musicStateSaveName);
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