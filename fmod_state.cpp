#include "extension.h"

#include <filesystem.h>

using namespace SourceHook;

/**
 * Save the current state of bank, event and global parameters to a .musicstate.sav file with the same name as the .sav file
 */
void SaveMusicState(const char* musicStateSaveName) {
    // IF WE'RE DOING AN AUTOSAVE: There is some more stuff to do beforehand
    if (strcmp(musicStateSaveName, "autosave.musicstate.sav") == 0) {
        // Shift all the possible autosaveXX.musicstate.sav files
        for (int i = 98; i >= 1; i--) {
            // Prepare the "XX" numbers on two digits
            char autosaveIndex[3]; 
            char buffer[3];
            itoa(i, buffer, 10);
            if (i < 10) {
                autosaveIndex[0] = '0';
                autosaveIndex[1] = buffer[0];
                autosaveIndex[2] = '\0';
            } else {
                strcpy(autosaveIndex, buffer);
            }
            // Prepare the "XX+1" numbers on two digits
            char autosaveIndexPlusOne[3]; 
            char bufferPlusOne[3];
            itoa(i+1, bufferPlusOne, 10);
            if (i+1 < 10) {
                autosaveIndexPlusOne[0] = '0';
                autosaveIndexPlusOne[1] = bufferPlusOne[0];
                autosaveIndexPlusOne[2] = '\0';
            } else {
                strcpy(autosaveIndexPlusOne, bufferPlusOne);
            }
            // Build the path to the file
            const char* pathPrefix = "save/autosave";
            const char* pathSuffix = ".musicstate.sav";
            size_t pathPrefixLength = strlen(pathPrefix);
            size_t pathSuffixLength = strlen(pathSuffix);
            size_t autosaveIndexLength = strlen(autosaveIndex);
            char* saveFullPath = new char[pathPrefixLength + autosaveIndexLength + pathSuffixLength + 1];
            strcpy(saveFullPath, pathPrefix);
            strcat(saveFullPath, autosaveIndex);
            strcat(saveFullPath, pathSuffix);
            // Build the path to the +1 file
            char* saveFullPathPlusOne = new char[pathPrefixLength + autosaveIndexLength + pathSuffixLength + 1];
            strcpy(saveFullPathPlusOne, pathPrefix);
            strcat(saveFullPathPlusOne, autosaveIndexPlusOne);
            strcat(saveFullPathPlusOne, pathSuffix);
            if (g_AdaptiveMusicExt.filesystem->FileExists(saveFullPath, "MOD")) {
                g_AdaptiveMusicExt.filesystem->RenameFile(saveFullPath, saveFullPathPlusOne, "MOD");
            }
        }
        if (g_AdaptiveMusicExt.filesystem->FileExists("save/autosave.musicstate.sav", "MOD")) {
            g_AdaptiveMusicExt.filesystem->RenameFile("save/autosave.musicstate.sav","save/autosave01.musicstate.sav", "MOD");
        }
    }
    // Build the path to the file
    const char* pathPrefix = "save/";
    size_t pathPrefixLength = strlen(pathPrefix);
    size_t saveNameLength = strlen(musicStateSaveName);
    char* saveFullPath = new char[pathPrefixLength + saveNameLength + 1];
    strcpy(saveFullPath, pathPrefix);
    strcat(saveFullPath, musicStateSaveName);
    // When opening the file and writing to it, it gets completely wiped first, so no need to wipe it beforehand
    FileHandle_t saveFileHandle = g_AdaptiveMusicExt.filesystem->Open(saveFullPath, "w", "MOD");
    if (saveFileHandle == nullptr) {
        META_CONPRINTF("AdaptiveMusic Plugin - Failed to open save file for writing: %s", saveFullPath);
        return;
    }
    META_CONPRINTF("AdaptiveMusic Plugin - Saving the Adaptive Music state to %s\n", saveFullPath);
    // Write the current state
    // BANK
    g_AdaptiveMusicExt.filesystem->Write("bank ", (strlen("bank ")), saveFileHandle);
    char* bankName = g_AdaptiveMusicExt.loadedFMODStudioBankName;
    g_AdaptiveMusicExt.filesystem->Write(bankName, (strlen(bankName)), saveFileHandle);  
    g_AdaptiveMusicExt.filesystem->Write("\n", 1, saveFileHandle);
    // EVENT
    g_AdaptiveMusicExt.filesystem->Write("event ", (strlen("event ")), saveFileHandle);
    char* eventPath = g_AdaptiveMusicExt.startedFMODStudioEventPath;
    g_AdaptiveMusicExt.filesystem->Write(eventPath, (strlen(eventPath)), saveFileHandle); 
    g_AdaptiveMusicExt.filesystem->Write("\n", 1, saveFileHandle);
    // TIMESTAMP
    int timelinePosition = g_AdaptiveMusicExt.GetCurrentFMODTimelinePosition();
    if (timelinePosition != -1){
        std::string timelinePositionString = std::to_string(timelinePosition);
        const char* timelinePositionConstChar = timelinePositionString.c_str();
        g_AdaptiveMusicExt.filesystem->Write("timestamp ", (strlen("timestamp ")), saveFileHandle);
        g_AdaptiveMusicExt.filesystem->Write(timelinePositionConstChar, (strlen(timelinePositionConstChar)), saveFileHandle); 
        g_AdaptiveMusicExt.filesystem->Write("\n", 1, saveFileHandle);
    }
    // PARAMETERS
    FMOD_STUDIO_PARAMETER_DESCRIPTION globalParameters[128];
    int parameterCount;
    FMOD_RESULT result; 
    result = g_AdaptiveMusicExt.fmodStudioSystem->getParameterDescriptionList(globalParameters, sizeof(globalParameters), &parameterCount);
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - Could not get the Global Parameter count. Error: (%d) %s\n", result, FMOD_ErrorString(result));
    } else {
        for (int i = 0; i < parameterCount; i++) {
            // Get the parameter value
            float parameterValue;
            FMOD_RESULT result; 
            result = g_AdaptiveMusicExt.fmodStudioSystem->getParameterByName(globalParameters[i].name, &parameterValue);
            if (result != FMOD_OK) {
                META_CONPRINTF("AdaptiveMusic Plugin - Could not get the Global Parameter value. Error: (%d) %s\n", result, FMOD_ErrorString(result));
            } else {
                // parameter (space)
                g_AdaptiveMusicExt.filesystem->Write("parameter ", (strlen("parameter ")), saveFileHandle);
                // parameter_name
                const char* parameterName = globalParameters[i].name;
                g_AdaptiveMusicExt.filesystem->Write(parameterName, (strlen(parameterName)), saveFileHandle); 
                // (space)
                g_AdaptiveMusicExt.filesystem->Write(" ", (strlen(" ")), saveFileHandle);
                // parameter_value
                std::string parameterValueString = std::to_string(parameterValue);
                const char* parameterValueConstChar = parameterValueString.c_str();
                g_AdaptiveMusicExt.filesystem->Write(parameterValueConstChar, (strlen(parameterValueConstChar)), saveFileHandle);
                g_AdaptiveMusicExt.filesystem->Write("\n", 1, saveFileHandle);
            }
        }
    }
    // Close the handle
    g_AdaptiveMusicExt.filesystem->Close(saveFileHandle);
}

/**
 * Restore the current state of bank, event and global parameters from a .kv file with the same name as the .sav file
 */
void RestoreMusicState(const char* musicStateSaveName) {
    // Build the path to the file
    const char* pathPrefix = "save/";
    size_t pathPrefixLength = strlen(pathPrefix);
    size_t saveNameLength = strlen(musicStateSaveName);
    char* saveFullPath = new char[pathPrefixLength + saveNameLength + 1];
    strcpy(saveFullPath, pathPrefix);
    strcat(saveFullPath, musicStateSaveName);
    FileHandle_t saveFileHandle = g_AdaptiveMusicExt.filesystem->Open(saveFullPath, "r", "MOD");
    if (saveFileHandle == nullptr) {
        META_CONPRINTF("AdaptiveMusic Plugin - Failed to open save file for reading: %s", saveFullPath);
        return;
    }
    META_CONPRINTF("AdaptiveMusic Plugin - Restoring the Adaptive Music state from %s\n", saveFullPath);
    // Read the current state
    char buf[512];
    g_AdaptiveMusicExt.filesystem->ReadLine(buf, sizeof(buf), saveFileHandle);
    while (strcmp(buf, "") != 0) {
        // READ THE LINE AND FILL THE TOKENS
        char* tokens[3] = {};
        char* token = strtok(buf, " ");
        int i = 0;
        while (token != nullptr) {
            // Remove backlash-n
            char* readPtr = token;
            char* writePtr = token;
            while (*readPtr != '\0') {
                if (*readPtr != '\n') {
                    *writePtr = *readPtr;
                    writePtr++;
                }
                readPtr++;
            }
            *writePtr = '\0'; // Null-terminate the modified string
            // Next token
            tokens[i] = token;
            token = strtok(nullptr, " ");
            i++;
        }
        // BANK
        if (strcmp(tokens[0], "bank") == 0 && tokens[1] != nullptr) {
            g_AdaptiveMusicExt.LoadFMODBank(tokens[1]);
        }
        // EVENT
        if (strcmp(tokens[0], "event") == 0 && tokens[1] != nullptr) {
            //g_AdaptiveMusicExt.StartFMODEvent(tokens[1]); // Don't start the event from the save file, the KeyValues parsing will
        }
        // TIMESTAMP
        if (strcmp(tokens[0], "timestamp") == 0 && tokens[1] != nullptr) {
            g_AdaptiveMusicExt.SetCurrentFMODTimelinePosition(atoi(tokens[1]));
        }
        // PARAMETERS
        if (strcmp(tokens[0], "parameter") == 0 && tokens[1] != nullptr && tokens[2] != nullptr) {
            g_AdaptiveMusicExt.SetFMODGlobalParameter(tokens[1], atof(tokens[2]));
        }
        g_AdaptiveMusicExt.filesystem->ReadLine(buf, sizeof(buf), saveFileHandle);
    }
    // Close the handle
    g_AdaptiveMusicExt.filesystem->Close(saveFileHandle);
}

/**
 * Helper function to replace the .sav file to a .musicstate.kv file
 */
const char* replaceSavWithMusicState(const char* original) {
    // The prefixes to be removed and suffixes to be replaced
    const char* prefix1 = "save\\";
    const char* prefix2 = "save/";
    const char* oldSuffix = ".sav";
    const char* newSuffix = ".musicstate.sav";
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

void SyncFMODSettings() {
    // Go through the client config file
    const char* configFilePath = "cfg/config.cfg";
    FileHandle_t configFileHandle = g_AdaptiveMusicExt.filesystem->Open(configFilePath, "r", "MOD");
    if (configFileHandle == nullptr) {
        META_CONPRINTF("AdaptiveMusic Plugin - Failed to open config file for reading: %s\n", configFilePath);
        return;
    }
    META_CONPRINTF("AdaptiveMusic Plugin - Syncing the FMOD settings from %s\n", configFilePath);
    char buf[512];
    g_AdaptiveMusicExt.filesystem->ReadLine(buf, sizeof(buf), configFileHandle);
    while (strcmp(buf, "") != 0) {
        // READ THE LINE AND FILL THE TOKENS
        char* tokens[3] = {};
        char* token = strtok(buf, " ");
        int i = 0;
        while (token != nullptr) {
            // Remove backlash-n
            char* readPtr = token;
            char* writePtr = token;
            while (*readPtr != '\0') {
                if (*readPtr != '\n') {
                    *writePtr = *readPtr;
                    writePtr++;
                }
                readPtr++;
            }
            *writePtr = '\0'; // Null-terminate the modified string
            // Next token
            tokens[i] = token;
            token = strtok(nullptr, " ");
            i++;
        }
        // Music Volume
        if (strcmp(tokens[0], "snd_musicvolume") == 0 && tokens[1] != nullptr) {
            // Remove quotes
            size_t len = strlen(tokens[1]);
            char* result = new char[len - 1];
            strncpy(result, tokens[1] + 1, len - 2);
            result[len - 2] = '\0';
            float volume = atof(result);
            g_AdaptiveMusicExt.SetFMODVolume(volume);
        }
        g_AdaptiveMusicExt.filesystem->ReadLine(buf, sizeof(buf), configFileHandle);
    }
    // Close the handle
    g_AdaptiveMusicExt.filesystem->Close(configFileHandle);
}

SH_DECL_HOOK1_void(IServerGameDLL, SaveGlobalState, SH_NOATTRIB, 0, CSaveRestoreData *);
SH_DECL_HOOK1_void(IServerGameDLL, RestoreGlobalState, SH_NOATTRIB, 0, CSaveRestoreData *);
SH_DECL_HOOK2_void(IServerGameDLL, Restore, SH_NOATTRIB, 0, CSaveRestoreData *, bool);

void Hook_SaveGlobalState(CSaveRestoreData *saveRestoreData)
{
    const char *saveName = engine->GetSaveFileName();
    const char *musicStateSaveName = replaceSavWithMusicState(saveName);
    SaveMusicState(musicStateSaveName);
    RETURN_META(MRES_HANDLED);
}

void Hook_RestoreGlobalState(CSaveRestoreData *saveRestoreData)
{
    const char * saveName = engine->GetMostRecentlyLoadedFileName();
    const char *musicStateSaveName = replaceSavWithMusicState(saveName);
    RestoreMusicState(musicStateSaveName);
    RETURN_META(MRES_HANDLED);
}

void Hook_Restore(CSaveRestoreData *saveRestoreData, bool)
{
    const char * saveName = engine->GetMostRecentlyLoadedFileName();
    const char *musicStateSaveName = replaceSavWithMusicState(saveName);
    RestoreMusicState(musicStateSaveName);
    RETURN_META(MRES_HANDLED);
}

void AddFMODStateHooks()
{
   SH_ADD_HOOK(IServerGameDLL, SaveGlobalState, gamedll, SH_STATIC(Hook_SaveGlobalState), false);
   SH_ADD_HOOK(IServerGameDLL, RestoreGlobalState, gamedll, SH_STATIC(Hook_RestoreGlobalState), false); // THIS DOES NOT TRIGGER ON FIRST LOAD
   SH_ADD_HOOK(IServerGameDLL, Restore, gamedll, SH_STATIC(Hook_Restore), false); // THIS DOES TRIGGER ON FIRST LOAD
}
 
void RemoveFMODStateHooks()
{
   SH_REMOVE_HOOK(IServerGameDLL, SaveGlobalState, gamedll, SH_STATIC(Hook_SaveGlobalState), false);
   SH_REMOVE_HOOK(IServerGameDLL, RestoreGlobalState, gamedll, SH_STATIC(Hook_RestoreGlobalState), false); // THIS DOES NOT TRIGGER ON FIRST LOAD
   SH_REMOVE_HOOK(IServerGameDLL, Restore, gamedll, SH_STATIC(Hook_Restore), false); // THIS DOES TRIGGER ON FIRST LOAD
}