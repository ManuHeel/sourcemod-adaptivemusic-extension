/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod AMM Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"
#include <filesystem.h>
#include <tier1/KeyValues.h>
#include <stdio.h>

// FMOD Includes
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include "fmod_errors.h"

// Personal includes
#include "fmod_state.cpp"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

// ----------------
// NATIVE FUNCTIONS
// ----------------

/**
 * SourceMod native function for AdaptiveMusicExt::LoadFMODBank
 */
cell_t LoadFMODBank(IPluginContext *pContext, const cell_t *params)
{
	char *bankName;

    pContext->LocalToString(params[1], &bankName);
    std::string bankNameStr(bankName);
    return g_AdaptiveMusicExt.LoadFMODBank(bankNameStr);
}

/**
 * SourceMod native function for AdaptiveMusicExt::StartFMODEvent
 */
cell_t StartFMODEvent(IPluginContext *pContext, const cell_t *params)
{
    char *eventPath;
    pContext->LocalToString(params[1], &eventPath);
    std::string eventPathStr(eventPath);
    return g_AdaptiveMusicExt.StartFMODEvent(eventPathStr);
}

/**
 * SourceMod native function for AdaptiveMusicExt::StopFMODEvent
 */
cell_t StopFMODEvent(IPluginContext *pContext, const cell_t *params)
{
    char *eventPath;
    pContext->LocalToString(params[1], &eventPath);
    std::string eventPathStr(eventPath);
    return g_AdaptiveMusicExt.StopFMODEvent(eventPathStr);
}

/**
 * SourceMod native function for AdaptiveMusicExt::SetFMODGlobalParameter
 */
cell_t SetFMODGlobalParameter(IPluginContext *pContext, const cell_t *params)
{
    char *parameterName;
    pContext->LocalToString(params[1], &parameterName);
    std::string parameterNameStr(parameterName);
    float value = sp_ctof(params[2]);
    return g_AdaptiveMusicExt.SetFMODGlobalParameter(parameterNameStr, value);
}

/**
 * SourceMod native function for AdaptiveMusicExt::SetFMODPausedState
 */
cell_t SetFMODPausedState(IPluginContext *pContext, const cell_t *params)
{
    int pausedState = params[1];
    return g_AdaptiveMusicExt.SetFMODPausedState(pausedState);
}

/**
 * Defining the native functions of the extensions
 */
const sp_nativeinfo_t MyNatives[] = 
{
    {"LoadFMODBank", LoadFMODBank},
    {"StartFMODEvent", StartFMODEvent},
    {"StopFMODEvent", StopFMODEvent},
    {"SetFMODGlobalParameter", SetFMODGlobalParameter},
    {"SetFMODPausedState", SetFMODPausedState},
    {NULL, NULL},
};

// -----------------------
// GENERAL EXTENSION SETUP
// -----------------------

bool AdaptiveMusicExt::SDK_OnLoad(char *error, size_t maxlen, bool late) {
    smutils->LogMessage(myself, "AMM Extension - SDK Loaded");
    StartFMODEngine();
    restoredTimelinePosition = 0;
    return true;
}

void AdaptiveMusicExt::SDK_OnAllLoaded() {
    sharesys->AddNatives(myself, MyNatives);
}

void AdaptiveMusicExt::SDK_OnUnload() {
    smutils->LogMessage(myself, "AMM Extension - SDK Unloaded");
}

bool AdaptiveMusicExt::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late) {
    META_CONPRINTF("AMM Extension - MetaMod Loaded \n");
    CreateInterfaceFn fileSystemFactory = ismm->GetFileSystemFactory();
    GET_V_IFACE_CURRENT(GetEngineFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);  
    AddFMODStateHooks();
    return true;
}

bool AdaptiveMusicExt::SDK_OnMetamodUnload(char *error, size_t maxlen) {
    META_CONPRINTF("AMM Extension - MetaMod Unloaded \n");
    RemoveFMODStateHooks();
    return true;
}

// --------------
// FMOD FUNCTIONS
// --------------


/**
 * Helper method to sanitize the name of an FMOD Bank, adding ".bank" if it's not already present
 * @param bankName The FMOD Bank name to sanitize
 * @return The sanitized Bank name (same as the initial if it was already ending with ".bank")
 */
std::string SanitizeBankName(const std::string &bankName) {
    const std::string bankExtension = ".bank";
    if (bankName.size() >= bankExtension.size() && bankName.compare(bankName.size() - bankExtension.size(), bankExtension.size(), bankExtension) == 0) {
        return bankName;
    }
    return bankName + bankExtension;
}

/**
 * Start the FMOD Studio System and initialize it
 * @return The error code (or 0 if no error was encountered)
 */
int AdaptiveMusicExt::StartFMODEngine() {
    FMOD_RESULT result;
    result = FMOD::Studio::System::create(&fmodStudioSystem);
    if (result != FMOD_OK) {
        META_CONPRINTF("AMM Extension - FMOD engine could not be created (%d): %s\n", result,
                       FMOD_ErrorString(result));
        return (result);
    }
    result = fmodStudioSystem->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK) {
        META_CONPRINTF("AMM Extension - FMOD engine could not initialize (%d): %s\n", result,
                       FMOD_ErrorString(result));
        return (result);
    }
    SyncFMODSettings(); // Sync the settings, volume etc
    META_CONPRINTF("AMM Extension - FMOD engine successfully started\n");
    return (0);
}

/**
 * Stop the FMOD Studio System
 * @return The error code (or 0 if no error was encountered)
 */
int AdaptiveMusicExt::StopFMODEngine() {
    FMOD_RESULT result;
    result = fmodStudioSystem->release();
    if (result != FMOD_OK) {
        META_CONPRINTF("AMM Extension - FMOD engine could not be released (%d): %s\n", result,
                       FMOD_ErrorString(result));
        return (result);
    }
    META_CONPRINTF("AMM Extension - FMOD engine successfully stopped\n");
    return (0);
}

/**
 * Get the path of a Bank file in the sound/fmod/banks folder from the GamePath
 * @param bankName The FMOD Bank name to locate
 * @return The FMOD Bank's full path from the file system
 */
std::string AdaptiveMusicExt::GetFMODBankPath(const std::string &bankName) {
    std::string sanitizedBankName = SanitizeBankName(bankName);
    std::string baseDir = g_SMAPI->GetBaseDir();
    std::string banksSubPath = "/sound/fmod/banks/";
    std::string bankPath = baseDir + banksSubPath + sanitizedBankName;
    // convert backward slashes to forward slashes
    std::replace(bankPath.begin(), bankPath.end(), '\\', '/');
    return bankPath;
}

/**
 * Load an FMOD Bank
 * @param bankName The name of the FMOD Bank to load
 * @return The error code (or 0 if no error was encountered)
 */
int AdaptiveMusicExt::LoadFMODBank(const std::string &bankName) {
    if (loadedFMODStudioBankName == bankName) {
        // Bank is already loaded
        META_CONPRINTF("AMM Extension - FMOD bank requested for loading but already loaded: %s\n", bankName.c_str());
    } else {
        // Load the requested bank
        std::string bankPath = GetFMODBankPath(bankName);
        FMOD_RESULT result;
        result = fmodStudioSystem->loadBankFile(bankPath.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &loadedFMODStudioBank);
        if (result != FMOD_OK) {
            META_CONPRINTF("AMM Extension - Could not load FMOD bank: %s. Error (%d): %s\n", bankName.c_str(), result, FMOD_ErrorString(result));
            return (-1);
        }
        std::string bankStringsName = bankName + ".strings";
        std::string bankStringsPath = GetFMODBankPath(bankStringsName);
        result = fmodStudioSystem->loadBankFile(bankStringsPath.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &loadedFMODStudioStringsBank);
        if (result != FMOD_OK) {
            META_CONPRINTF("AMM Extension - Could not load FMOD bank: %s. Error (%d): %\n", bankStringsName.c_str(), result, FMOD_ErrorString(result));
            return (-1);
        }
        META_CONPRINTF("AMM Extension - Bank successfully loaded: %s\n", bankName.c_str());
        loadedFMODStudioBankName = bankName;
    }
    return (0);
}

/**
 * Start an FMOD Event
 * @param eventPath The name of the FMOD Event to start
 * @return The error code (or 0 if no error was encountered)
 */
int AdaptiveMusicExt::StartFMODEvent(const std::string& eventPath) {
    if (!startedFMODStudioEventPath.empty() && (eventPath == startedFMODStudioEventPath)) {
        // Event is already loaded
        META_CONPRINTF("AdaptiveMusic Plugin - Event requested for starting but already started (%s)\n", eventPath.c_str());
        // However, if there's a restored timeline position from a save file, use it as we may be reloading from the same map (autosave, etc)
        if (restoredTimelinePosition != 0) {
            createdFMODStudioEventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
            fmodStudioSystem->update();
            createdFMODStudioEventInstance->setTimelinePosition(restoredTimelinePosition);
            restoredTimelinePosition = 0;
            createdFMODStudioEventInstance->start();
            fmodStudioSystem->update();
        }
    } else {
        // Event is new
        if (!startedFMODStudioEventPath.empty()) {
            // Stop the currently playing event
            StopFMODEvent(startedFMODStudioEventPath);
        }

        const std::string eventPathPrefix = "event:/";
        std::string fullEventPath = eventPathPrefix + eventPath;
        FMOD_RESULT result;
        result = fmodStudioSystem->getEvent(fullEventPath.c_str(), &startedFMODStudioEventDescription);
        result = startedFMODStudioEventDescription->createInstance(&createdFMODStudioEventInstance);
        
        // If there's a restored timeline position from a save file, use it
        if (restoredTimelinePosition != 0) {
            createdFMODStudioEventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
            fmodStudioSystem->update();
            createdFMODStudioEventInstance->setTimelinePosition(restoredTimelinePosition);
            restoredTimelinePosition = 0;
        }
        
        result = createdFMODStudioEventInstance->start();
        fmodStudioSystem->update();
        
        if (result != FMOD_OK) {
            META_CONPRINTF("AdaptiveMusic Plugin - Could not start Event (%s). Error: (%d) %s\n", eventPath.c_str(), result,
                           FMOD_ErrorString(result));
            return (-1);
        }
        
        META_CONPRINTF("AdaptiveMusic Plugin - Event successfully started (%s)\n", eventPath.c_str());
        startedFMODStudioEventPath = eventPath; // Assign directly, no need for new char array
    }
    return (0);
}

/**
 * Get the current timeline position of the running event instance
 * @return The current timeline position of the running event
 */
int AdaptiveMusicExt::GetCurrentFMODTimelinePosition() {
    if (g_AdaptiveMusicExt.createdFMODStudioEventInstance == nullptr) {
        META_CONPRINTF("AMM Extension - Asking for the current event instance timeline position but no event is running\n");
        return -1;
    }
    FMOD_RESULT result;
    int timelinePosition;
    result = g_AdaptiveMusicExt.createdFMODStudioEventInstance->getTimelinePosition(&timelinePosition);
    if (result != FMOD_OK) {
        META_CONPRINTF("AMM Extension - Could not find the timeline position from the event %s. Error: (%d) %s\n", g_AdaptiveMusicExt.startedFMODStudioEventPath, result, FMOD_ErrorString(result));
        return -1;
    } else {
        return timelinePosition;
    }
}

/**
 * Setp the current timeline position of the running event instance
 */
void AdaptiveMusicExt::SetCurrentFMODTimelinePosition(int timelinePosition) {
    /*
    if (g_AdaptiveMusicExt.createdFMODStudioEventInstance == nullptr) {
        META_CONPRINTF("AMM Extension - Asking to update the current event instance timeline position but \n");
    }
    FMOD_RESULT result;
    result = g_AdaptiveMusicExt.createdFMODStudioEventInstance->setTimelinePosition(timelinePosition);
    if (result != FMOD_OK) {
        META_CONPRINTF("AMM Extension - Could not find the timeline position from the event %s. Error: (%d) %s\n", g_AdaptiveMusicExt.startedFMODStudioEventPath, result, FMOD_ErrorString(result));
    }
    */
    // ONLY SET THE VARIABLE
    restoredTimelinePosition = timelinePosition;
}

/**
 * Stop an FMOD Event
 * @param eventPath The name of the FMOD Event to stop
 * @return The error code (or 0 if no error was encountered)
 */
int AdaptiveMusicExt::StopFMODEvent(const std::string &eventPath) {
    std::string eventPathPrefix = "event:/";
    std::string fullEventPath = eventPathPrefix + eventPath; // Concatenate the prefix and event path
    FMOD_RESULT result;
    result = fmodStudioSystem->getEvent(fullEventPath.c_str(), &startedFMODStudioEventDescription);
    result = startedFMODStudioEventDescription->releaseAllInstances();
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        META_CONPRINTF("AMM Extension - Could not stop Event (%s). Error: (%d) %s\n", eventPath.c_str(), result, FMOD_ErrorString(result));
        return -1;
    }
    META_CONPRINTF("AMM Extension - Event successfully stopped (%s)\n", eventPath.c_str());
    startedFMODStudioEventPath.clear();
    return 0;
}

/**
 * Set the value for a global FMOD Parameter
 * @param parameterName The name of the FMOD Parameter to set
 * @param value The value to set the FMOD Parameter to
 * @return The error code (or 0 if no error was encountered)
 */
int AdaptiveMusicExt::SetFMODGlobalParameter(const std::string &parameterName, float value) {
    FMOD_RESULT result;
    result = fmodStudioSystem->setParameterByName(parameterName.c_str(), value);
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        META_CONPRINTF("AMM Extension - Could not set Global Parameter value (%s) (%f). Error: (%d) %s\n",
                       parameterName.c_str(), value, result, FMOD_ErrorString(result));
        return -1;
    }
    META_CONPRINTF("AMM Extension - Global Parameter %s set to %f\n", parameterName.c_str(), value);
    return 0;
}

/**
 * Get all the parameters registered in the bank
 * @return An array of all parameters registered in the bank
 */
std::vector<FMOD_STUDIO_PARAMETER_DESCRIPTION> AdaptiveMusicExt::GetAllFMODGlobalParameters() {
    FMOD_RESULT result;
    FMOD_STUDIO_PARAMETER_DESCRIPTION globalParameters[128];
    int parameterCount;
    result = fmodStudioSystem->getParameterDescriptionList(globalParameters, sizeof(globalParameters), &parameterCount);
    if (result != FMOD_OK) {
        META_CONPRINTF("AMM Extension - Could not get the Global Parameter count. Error: (%d) %s\n", result, FMOD_ErrorString(result));
        return {}; // Return an empty vector in case of error
    } else {
        // Create a vector to hold the parameters
        std::vector<FMOD_STUDIO_PARAMETER_DESCRIPTION> limitedGlobalParameters(globalParameters, globalParameters + parameterCount);
        return limitedGlobalParameters; // Return the vector
    }
}

/**
 * Pause/Unpause the playback of the engine
 * @param pausedState true if the desired state of the playback is "paused", false otherwise
 * @return The error code (or 0 if no error was encountered)
 */
int AdaptiveMusicExt::SetFMODPausedState(bool pausedState) {
    META_CONPRINTF("AMM Extension - Setting the FMOD master bus paused state to %d\n", pausedState);
    FMOD::Studio::Bus *bus = nullptr; // Initialize bus pointer to nullptr
    FMOD_RESULT result;
    result = fmodStudioSystem->getBus("bus:/", &bus);
    if (result != FMOD_OK) {
        META_CONPRINTF("AMM Extension - Could not find the FMOD master bus! (%d) %s\n", result, FMOD_ErrorString(result));
        return -1;
    }
    result = bus->setPaused(pausedState);
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        META_CONPRINTF("AMM Extension - Could not pause the FMOD master bus! (%d) %s\n", result, FMOD_ErrorString(result));
        return -1;
    }
    knownFMODPausedState = pausedState;
    // When leaving the paused state, it's a good measure to sync the settings, volume etc, to take into account the potential modifications made
    if (!pausedState) {
        SyncFMODSettings();
    }

    return 0;
}

/**
 * Set the master bus volume
 * @param volume the desired volume from 0.0 to 1.0
 * @return The error code (or 0 if no error was encountered)
 */
int AdaptiveMusicExt::SetFMODVolume(float volume) {
    META_CONPRINTF("AMM Extension - Setting the FMOD volume to %f\n", volume);
    FMOD::Studio::Bus *bus = nullptr; // Initialize bus pointer to nullptr
    FMOD_RESULT result;
    result = fmodStudioSystem->getBus("bus:/", &bus);
    if (result != FMOD_OK) {
        META_CONPRINTF("AMM Extension - Could not find the FMOD master bus! (%d) %s\n", result, FMOD_ErrorString(result));
        return -1;
    }
    result = bus->setVolume(volume);
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        META_CONPRINTF("AMM Extension - Could not set the FMOD master bus volume! (%d) %s\n", result, FMOD_ErrorString(result));
        return -1;
    }

    return 0;
}

SMEXT_LINK(&g_AdaptiveMusicExt);
