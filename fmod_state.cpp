#include "extension.h"

#include <filesystem.h>

using namespace SourceHook;

/**
 * Save the current state of bank, event and global parameters to a .musicstate.sav file with the same name as the .sav file
 */
void SaveMusicState(const std::string& musicStateSaveName) {
    // IF WE'RE DOING AN AUTOSAVE: There is some more stuff to do beforehand
    if (musicStateSaveName == "autosave.musicstate.sav") {
        // Shift all the possible autosaveXX.musicstate.sav files
        for (int i = 98; i >= 1; i--) {
            // Prepare the "XX" numbers on two digits
            std::string autosaveIndex = (i < 10) ? "0" + std::to_string(i) : std::to_string(i);
            std::string autosaveIndexPlusOne = (i + 1 < 10) ? "0" + std::to_string(i + 1) : std::to_string(i + 1);

            // Build the path to the file
            std::string saveFullPath = "save/autosave" + autosaveIndex + ".musicstate.sav";
            std::string saveFullPathPlusOne = "save/autosave" + autosaveIndexPlusOne + ".musicstate.sav";

            if (g_AdaptiveMusicExt.filesystem->FileExists(saveFullPath.c_str(), "MOD")) {
                g_AdaptiveMusicExt.filesystem->RenameFile(saveFullPath.c_str(), saveFullPathPlusOne.c_str(), "MOD");
            }
        }
        if (g_AdaptiveMusicExt.filesystem->FileExists("save/autosave.musicstate.sav", "MOD")) {
            g_AdaptiveMusicExt.filesystem->RenameFile("save/autosave.musicstate.sav", "save/autosave01.musicstate.sav", "MOD");
        }
    }

    // Build the path to the file
    std::string saveFullPath = "save/" + musicStateSaveName;

    // When opening the file and writing to it, it gets completely wiped first, so no need to wipe it beforehand
    FileHandle_t saveFileHandle = g_AdaptiveMusicExt.filesystem->Open(saveFullPath.c_str(), "w", "MOD");
    if (saveFileHandle == nullptr) {
        META_CONPRINTF("AMM Extension - Failed to open save file for writing: %s", saveFullPath.c_str());
        return;
    }
    META_CONPRINTF("AMM Extension - Saving the Adaptive Music state to %s\n", saveFullPath.c_str());

    // Write the current state
    // BANK
    g_AdaptiveMusicExt.filesystem->Write("bank ", strlen("bank "), saveFileHandle);
    std::string bankName = g_AdaptiveMusicExt.loadedFMODStudioBankName;
    g_AdaptiveMusicExt.filesystem->Write(bankName.c_str(), bankName.length(), saveFileHandle);
    g_AdaptiveMusicExt.filesystem->Write("\n", 1, saveFileHandle);

    // EVENT
    g_AdaptiveMusicExt.filesystem->Write("event ", strlen("event "), saveFileHandle);
    std::string eventPath = g_AdaptiveMusicExt.startedFMODStudioEventPath;
    g_AdaptiveMusicExt.filesystem->Write(eventPath.c_str(), eventPath.length(), saveFileHandle);
    g_AdaptiveMusicExt.filesystem->Write("\n", 1, saveFileHandle);

    // TIMESTAMP
    int timelinePosition = g_AdaptiveMusicExt.GetCurrentFMODTimelinePosition();
    if (timelinePosition != -1) {
        std::string timelinePositionString = std::to_string(timelinePosition);
        g_AdaptiveMusicExt.filesystem->Write("timestamp ", strlen("timestamp "), saveFileHandle);
        g_AdaptiveMusicExt.filesystem->Write(timelinePositionString.c_str(), timelinePositionString.length(), saveFileHandle);
        g_AdaptiveMusicExt.filesystem->Write("\n", 1, saveFileHandle);
    }

    // PARAMETERS
    FMOD_STUDIO_PARAMETER_DESCRIPTION globalParameters[128];
    int parameterCount;
    FMOD_RESULT result; 
    result = g_AdaptiveMusicExt.fmodStudioSystem->getParameterDescriptionList(globalParameters, sizeof(globalParameters), &parameterCount);
    if (result != FMOD_OK) {
        META_CONPRINTF("AMM Extension - Could not get the Global Parameter count. Error: (%d) %s\n", result, FMOD_ErrorString(result));
    } else {
        for (int i = 0; i < parameterCount; i++) {
            // Get the parameter value
            float parameterValue;
            FMOD_RESULT result; 
            result = g_AdaptiveMusicExt.fmodStudioSystem->getParameterByName(globalParameters[i].name, &parameterValue);
            if (result != FMOD_OK) {
                META_CONPRINTF("AMM Extension - Could not get the Global Parameter value. Error: (%d) %s\n", result, FMOD_ErrorString(result));
            } else {
                // parameter (space)
                g_AdaptiveMusicExt.filesystem->Write("parameter ", strlen("parameter "), saveFileHandle);
                // parameter_name
                std::string parameterName = globalParameters[i].name;
                g_AdaptiveMusicExt.filesystem->Write(parameterName.c_str(), parameterName.length(), saveFileHandle);
                // (space)
                g_AdaptiveMusicExt.filesystem->Write(" ", strlen(" "), saveFileHandle);
                // parameter_value
                std::string parameterValueString = std::to_string(parameterValue);
                g_AdaptiveMusicExt.filesystem->Write(parameterValueString.c_str(), parameterValueString.length(), saveFileHandle);
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
void RestoreMusicState(const std::string& musicStateSaveName) {
    // Build the path to the file
    std::string saveFullPath = "save/" + musicStateSaveName;
    
    FileHandle_t saveFileHandle = g_AdaptiveMusicExt.filesystem->Open(saveFullPath.c_str(), "r", "MOD");
    if (saveFileHandle == nullptr) {
        META_CONPRINTF("AMM Extension - Failed to open save file for reading: %s\n", saveFullPath.c_str());
        return;
    }
    META_CONPRINTF("AMM Extension - Restoring the Adaptive Music state from %s\n", saveFullPath.c_str());

    // Read the current state
    char buf[512];
    g_AdaptiveMusicExt.filesystem->ReadLine(buf, sizeof(buf), saveFileHandle);
    while (std::strcmp(buf, "") != 0) {
        // READ THE LINE AND FILL THE TOKENS
        std::vector<std::string> tokens;
        char* token = std::strtok(buf, " ");
        while (token != nullptr) {
            std::string tokenStr = token;
            // Remove newline characters
            tokenStr.erase(std::remove(tokenStr.begin(), tokenStr.end(), '\n'), tokenStr.end());
            tokens.push_back(tokenStr);
            token = std::strtok(nullptr, " ");
        }

        // BANK
        if (tokens.size() > 1 && tokens[0] == "bank") {
            g_AdaptiveMusicExt.LoadFMODBank(tokens[1].c_str());
        }

        // EVENT
        if (tokens.size() > 1 && tokens[0] == "event") {
            // g_AdaptiveMusicExt.StartFMODEvent(tokens[1].c_str()); 
            // Don't start the event from the save file, the KeyValues parsing will
        }

        // TIMESTAMP
        if (tokens.size() > 1 && tokens[0] == "timestamp") {
            g_AdaptiveMusicExt.SetCurrentFMODTimelinePosition(std::stoi(tokens[1]));
        }

        // PARAMETERS
        if (tokens.size() > 2 && tokens[0] == "parameter") {
            g_AdaptiveMusicExt.SetFMODGlobalParameter(tokens[1].c_str(), std::stof(tokens[2]));
        }

        g_AdaptiveMusicExt.filesystem->ReadLine(buf, sizeof(buf), saveFileHandle);
    }

    // Close the handle
    g_AdaptiveMusicExt.filesystem->Close(saveFileHandle);
}

/**
 * Helper function to replace the .sav file to a .musicstate.kv file
 */
std::string replaceSavWithMusicState(const std::string& original) {
    // The prefixes to be removed and suffixes to be replaced
    const std::string prefix1 = "save\\";
    const std::string prefix2 = "save/";
    const std::string oldSuffix = ".sav";
    const std::string newSuffix = ".musicstate.sav";

    // Pointer to the start of the actual data after prefix removal
    std::string dataStart = original;

    // Check and remove the prefix if present
    if (dataStart.find(prefix1) == 0) {
        dataStart = dataStart.substr(prefix1.length());
    } else if (dataStart.find(prefix2) == 0) {
        dataStart = dataStart.substr(prefix2.length());
    }

    // Check if the string ends with ".sav"
    if (dataStart.length() < oldSuffix.length() || dataStart.substr(dataStart.length() - oldSuffix.length()) != oldSuffix) {
        // If not, return the string without the prefix
        return dataStart;
    }

    // Replace ".sav" with ".musicstate.sav"
    return dataStart.substr(0, dataStart.length() - oldSuffix.length()) + newSuffix;
}

void SyncFMODSettings() {
    // Go through the client config file
    std::string configFilePath = "cfg/config.cfg";
    FileHandle_t configFileHandle = g_AdaptiveMusicExt.filesystem->Open(configFilePath.c_str(), "r", "MOD");
    if (configFileHandle == nullptr) {
        META_CONPRINTF("AMM Extension - Failed to open config file for reading: %s\n", configFilePath.c_str());
        return;
    }
    META_CONPRINTF("AMM Extension - Syncing the FMOD settings from %s\n", configFilePath.c_str());
    char buf[512];
    g_AdaptiveMusicExt.filesystem->ReadLine(buf, sizeof(buf), configFileHandle);
    while (std::strcmp(buf, "") != 0) {
        // READ THE LINE AND FILL THE TOKENS
        std::vector<std::string> tokens;
        char* token = std::strtok(buf, " ");
        while (token != nullptr) {
            std::string tokenStr = token;
            // Remove backlash-n
            tokenStr.erase(std::remove(tokenStr.begin(), tokenStr.end(), '\n'), tokenStr.end());
            // Next token
            tokens.push_back(tokenStr);
            token = std::strtok(nullptr, " ");
        }
        // Music Volume
        if (tokens.size() > 1 && tokens[0] == "snd_musicvolume") {
            // Remove quotes
            if (tokens[1].size() > 2) {
                std::string result = tokens[1].substr(1, tokens[1].size() - 2);
                float volume = std::stof(result);
                g_AdaptiveMusicExt.SetFMODVolume(volume);
            }
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
    std::string saveName = engine->GetSaveFileName();
    std::string musicStateSaveName = replaceSavWithMusicState(saveName);
    SaveMusicState(musicStateSaveName);
    RETURN_META(MRES_HANDLED);
}

void Hook_RestoreGlobalState(CSaveRestoreData *saveRestoreData)
{
    std::string saveName = engine->GetMostRecentlyLoadedFileName();
    std::string musicStateSaveName = replaceSavWithMusicState(saveName);
    RestoreMusicState(musicStateSaveName);
    RETURN_META(MRES_HANDLED);
}

void Hook_Restore(CSaveRestoreData *saveRestoreData, bool)
{
    std::string saveName = engine->GetMostRecentlyLoadedFileName();
    std::string musicStateSaveName = replaceSavWithMusicState(saveName);
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
