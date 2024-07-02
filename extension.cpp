/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Adaptive Music Extension
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
	return g_AdaptiveMusicExt.LoadFMODBank(bankName);
}

/**
 * SourceMod native function for AdaptiveMusicExt::StartFMODEvent
 */
cell_t StartFMODEvent(IPluginContext *pContext, const cell_t *params)
{
	char *eventPath;
	pContext->LocalToString(params[1], &eventPath);
	return g_AdaptiveMusicExt.StartFMODEvent(eventPath);
}

/**
 * SourceMod native function for AdaptiveMusicExt::StopFMODEvent
 */
cell_t StopFMODEvent(IPluginContext *pContext, const cell_t *params)
{
	char *eventPath;
	pContext->LocalToString(params[1], &eventPath);
	return g_AdaptiveMusicExt.StopFMODEvent(eventPath);
}

/**
 * SourceMod native function for AdaptiveMusicExt::SetFMODGlobalParameter
 */
cell_t SetFMODGlobalParameter(IPluginContext *pContext, const cell_t *params)
{
	char *parameterName;
	pContext->LocalToString(params[1], &parameterName);
    float value = sp_ctof(params[2]);
	return g_AdaptiveMusicExt.SetFMODGlobalParameter(parameterName, value);
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
    smutils->LogMessage(myself, "Adaptive Music Extension - SDK Loaded");
    StartFMODEngine();
    restoredTimelinePosition = 0;
    return true;
}

void AdaptiveMusicExt::SDK_OnAllLoaded() {
    sharesys->AddNatives(myself, MyNatives);
}

void AdaptiveMusicExt::SDK_OnUnload() {
    smutils->LogMessage(myself, "Adaptive Music Extension - SDK Unloaded");
}

bool AdaptiveMusicExt::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late) {
    META_CONPRINTF("Adaptive Music Extension - MetaMod Loaded \n");
    CreateInterfaceFn fileSystemFactory = ismm->GetFileSystemFactory();
    GET_V_IFACE_CURRENT(GetEngineFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);  
    AddFMODStateHooks();
    return true;
}

bool AdaptiveMusicExt::SDK_OnMetamodUnload(char *error, size_t maxlen) {
    META_CONPRINTF("Adaptive Music Extension - MetaMod Unloaded \n");
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
const char *SanitizeBankName(const char *bankName) {
    const char *bankExtension = ".bank";
    size_t bankNameLength = strlen(bankName);
    size_t bankExtensionLength = strlen(bankExtension);
    if (bankNameLength >= bankExtensionLength && strcmp(bankName + bankNameLength - bankExtensionLength, bankExtension) == 0) {
        return bankName;
    }
    char* sanitizedBankName = new char[bankNameLength + bankExtensionLength + 1];
    strcpy(sanitizedBankName, bankName);
    strcat(sanitizedBankName, bankExtension);
    return (sanitizedBankName);
}

/**
 * Start the FMOD Studio System and initialize it
 * @return The error code (or 0 if no error was encountered)
 */
int AdaptiveMusicExt::StartFMODEngine() {
    FMOD_RESULT result;
    result = FMOD::Studio::System::create(&fmodStudioSystem);
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine could not be created (%d): %s\n", result,
                       FMOD_ErrorString(result));
        return (result);
    }
    result = fmodStudioSystem->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine could not initialize (%d): %s\n", result,
                       FMOD_ErrorString(result));
        return (result);
    }
    META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine successfully started\n");
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
        META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine could not be released (%d): %s\n", result,
                       FMOD_ErrorString(result));
        return (result);
    }
    META_CONPRINTF("AdaptiveMusic Plugin - FMOD engine successfully stopped\n");
    return (0);
}

/**
 * Get the path of a Bank file in the sound/fmod/banks folder from the GamePath
 * @param bankName The FMOD Bank name to locate
 * @return The FMOD Bank's full path from the file system
 */
const char * AdaptiveMusicExt::GetFMODBankPath(const char *bankName) {
    const char *sanitizedBankName = SanitizeBankName(bankName);
    const char *baseDir = g_SMAPI->GetBaseDir();
    const char *banksSubPath = "/sound/fmod/banks/";
    size_t sanitizedBankNameLength = strlen(sanitizedBankName);
    size_t baseDirLength = strlen(baseDir);
    size_t banksSubPathLength = strlen(banksSubPath);
    char* bankPath = new char[sanitizedBankNameLength + baseDirLength + banksSubPathLength + 1];
    strcpy(bankPath, baseDir);
    strcat(bankPath, banksSubPath);
    strcat(bankPath, sanitizedBankName);
    // convert backwards slashes to forward slashes
    for (int i = 0; i < strlen(bankPath) ; i++) {
        if (bankPath[i] == '\\')
            bankPath[i] = '/';
    }
    return bankPath;
}

/**
 * Load an FMOD Bank
 * @param bankName The name of the FMOD Bank to load
 * @return The error code (or 0 if no error was encountered)
 */
int AdaptiveMusicExt::LoadFMODBank(const char *bankName) {
    if (loadedFMODStudioBankName != nullptr && (strcmp(bankName, loadedFMODStudioBankName) == 0)) {
        // Bank is already loaded
        META_CONPRINTF("AdaptiveMusic Plugin - FMOD bank requested for loading but already loaded: %s\n", bankName);
    } else {
        // Load the requested bank
        const char *bankPath = GetFMODBankPath(bankName);
        FMOD_RESULT result;
        result = fmodStudioSystem->loadBankFile(bankPath, FMOD_STUDIO_LOAD_BANK_NORMAL,
                                                &loadedFMODStudioBank);
        if (result != FMOD_OK) {
            META_CONPRINTF("AdaptiveMusic Plugin - Could not load FMOD bank: %s. Error (%d): %s\n", bankName, result,
                           FMOD_ErrorString(result));
            return (-1);
        }
        const char *bankStringsSuffix = ".strings";
        size_t bankNameLength = strlen(bankName);
        size_t bankStringsSuffixLength = strlen(bankStringsSuffix);
        char* bankStringsName = new char[bankNameLength + bankStringsSuffixLength + 1];
        strcpy(bankStringsName, bankName);
        strcat(bankStringsName, bankStringsSuffix);
        const char *bankStringsPath = GetFMODBankPath(bankStringsName);
        result = fmodStudioSystem->loadBankFile(bankStringsPath,
                                                FMOD_STUDIO_LOAD_BANK_NORMAL,
                                                &loadedFMODStudioStringsBank);
        if (result != FMOD_OK) {
            META_CONPRINTF("AdaptiveMusic Plugin - Could not load FMOD bank: %s. Error (%d): %\n", bankStringsName,
                           result,
                           FMOD_ErrorString(result));
            return (-1);
        }
        META_CONPRINTF("AdaptiveMusic Plugin - Bank successfully loaded: %s\n", bankName);
        delete[] loadedFMODStudioBankName;
        loadedFMODStudioBankName = new char[strlen(bankName) + 1];
        strcpy(loadedFMODStudioBankName, bankName);
    }
    return (0);
}

/**
 * Start an FMOD Event
 * @param eventPath The name of the FMOD Event to start
 * @return The error code (or 0 if no error was encountered)
 */
int AdaptiveMusicExt::StartFMODEvent(const char *eventPath) {
    if (startedFMODStudioEventPath != nullptr && (strcmp(eventPath, startedFMODStudioEventPath) == 0)) {
        // Event is already loaded
        META_CONPRINTF("AdaptiveMusic Plugin - Event requested for starting but already started (%s)\n", eventPath);
        // However, if there's a restored timeline position from a save file, use it as we may be reloading from the same map (autosave, etc)
        if (restoredTimelinePosition != 0) {
            createdFMODStudioEventInstance->setTimelinePosition(restoredTimelinePosition);
            restoredTimelinePosition = 0;
        }
    } else {
        // Event is new
        if (startedFMODStudioEventPath != nullptr && (strcmp(startedFMODStudioEventPath, "") != 0)) {
            // Stop the currently playing event
            StopFMODEvent(startedFMODStudioEventPath);
        }
        const char *eventPathPrefix = "event:/";
        size_t eventPathLength = strlen(eventPath);
        size_t eventPathPrefixLength = strlen(eventPathPrefix);
        char* fullEventPath = new char[eventPathLength + eventPathPrefixLength + 1];
        strcpy(fullEventPath, eventPathPrefix);
        strcat(fullEventPath, eventPath);
        FMOD_RESULT result;
        result = fmodStudioSystem->getEvent(fullEventPath, &startedFMODStudioEventDescription);
        result = startedFMODStudioEventDescription->createInstance(&createdFMODStudioEventInstance);
        result = createdFMODStudioEventInstance->start();
        // If there's a restored timeline position from a save file, use it
        if (restoredTimelinePosition != 0) {
            createdFMODStudioEventInstance->setTimelinePosition(restoredTimelinePosition);
            restoredTimelinePosition = 0;
        }
        fmodStudioSystem->update();
        if (result != FMOD_OK) {
            META_CONPRINTF("AdaptiveMusic Plugin - Could not start Event (%s). Error: (%d) %s\n", eventPath, result,
                           FMOD_ErrorString(result));
            return (-1);
        }
        META_CONPRINTF("AdaptiveMusic Plugin - Event successfully started (%s)\n", eventPath);
        delete[] startedFMODStudioEventPath;
        startedFMODStudioEventPath = new char[strlen(eventPath) + 1];
        strcpy(startedFMODStudioEventPath, eventPath);
    }
    return (0);
}

/**
 * Get the current timeline position of the running event instance
 * @return The current timeline position of the running event
 */
int AdaptiveMusicExt::GetCurrentFMODTimelinePosition() {
    if (g_AdaptiveMusicExt.createdFMODStudioEventInstance == nullptr) {
        META_CONPRINTF("AdaptiveMusic Plugin - Asking for the current event instance timeline position but no event is running");
        return -1;
    }
    FMOD_RESULT result;
    int timelinePosition;
    result = g_AdaptiveMusicExt.createdFMODStudioEventInstance->getTimelinePosition(&timelinePosition);
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - Could not find the timeline position from the event %s. Error: (%d) %s\n", g_AdaptiveMusicExt.startedFMODStudioEventPath, result, FMOD_ErrorString(result));
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
        META_CONPRINTF("AdaptiveMusic Plugin - Asking to update the current event instance timeline position but no event is running");
    }
    FMOD_RESULT result;
    result = g_AdaptiveMusicExt.createdFMODStudioEventInstance->setTimelinePosition(timelinePosition);
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - Could not find the timeline position from the event %s. Error: (%d) %s\n", g_AdaptiveMusicExt.startedFMODStudioEventPath, result, FMOD_ErrorString(result));
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
int AdaptiveMusicExt::StopFMODEvent(const char *eventPath) {
    const char *eventPathPrefix = "event:/";
    size_t eventPathLength = strlen(eventPath);
    size_t eventPathPrefixLength = strlen(eventPathPrefix);
    char* fullEventPath = new char[eventPathLength + eventPathPrefixLength + 1];
    strcpy(fullEventPath, eventPathPrefix);
    strcat(fullEventPath, eventPath);
    FMOD_RESULT result;
    result = fmodStudioSystem->getEvent(fullEventPath, &startedFMODStudioEventDescription);
    result = startedFMODStudioEventDescription->releaseAllInstances();
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - Could not stop Event (%s). Error: (%d) %s\n", eventPath, result,
                       FMOD_ErrorString(result));
        return (-1);
    }
    META_CONPRINTF("AdaptiveMusic Plugin - Event successfully stopped (%s)\n", eventPath);
    delete[] startedFMODStudioEventPath;
    startedFMODStudioEventPath = new char[strlen("") + 1];
    strcpy(startedFMODStudioEventPath, "");
    return (0);
}

/**
 * Set the value for a global FMOD Parameter
 * @param parameterName The name of the FMOD Parameter to set
 * @param value The value to set the FMOD Parameter to
 * @return The error code (or 0 if no error was encountered)
 */
int AdaptiveMusicExt::SetFMODGlobalParameter(const char *parameterName, float value) {
    FMOD_RESULT result;
    result = fmodStudioSystem->setParameterByName(parameterName, value);
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - Could not set Global Parameter value (%s) (%f). Error: (%d) %s\n",
                       parameterName, value,
                       result, FMOD_ErrorString(result));
        return (-1);
    }
    META_CONPRINTF("AdaptiveMusic Plugin - Global Parameter %s set to %f\n", parameterName, value);
    return (0);
}

/**
 * Get all the parameters registered in the bank
 * @return An array of all parameters registered in the bank
 */
FMOD_STUDIO_PARAMETER_DESCRIPTION *AdaptiveMusicExt::GetAllFMODGlobalParameters(){
    FMOD_RESULT result; 
    FMOD_STUDIO_PARAMETER_DESCRIPTION globalParameters[128];
    int parameterCount;
    result = fmodStudioSystem->getParameterDescriptionList(globalParameters, sizeof(globalParameters), &parameterCount);
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - Could not get the Global Parameter count. Error: (%d) %s\n", result, FMOD_ErrorString(result));
        return nullptr;
    } else {
        // Strip the array of the empty cells
        // Allocate memory for the new array
        FMOD_STUDIO_PARAMETER_DESCRIPTION* limitedGlobalParameters = new FMOD_STUDIO_PARAMETER_DESCRIPTION[parameterCount];
        // Copy the elements from globalParameters to newArray
        for (int i = 0; i < parameterCount; i++) {
            limitedGlobalParameters[i] = globalParameters[i];
        }
        // Return the new array
        return limitedGlobalParameters;
    }
}

/**
 * Pause/Unpause the playback of the engine
 * @param pausedState true if the desired state of the playback is "paused", false otherwise
 * @return The error code (or 0 if no error was encountered)
 */
int AdaptiveMusicExt::SetFMODPausedState(bool pausedState) {
    META_CONPRINTF("AdaptiveMusic Plugin - Setting the FMOD master bus paused state to %d\n", pausedState);
    FMOD::Studio::Bus *bus;
    FMOD_RESULT result;
    result = fmodStudioSystem->getBus("bus:/", &bus);
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - Could not find the FMOD master bus! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    result = bus->setPaused(pausedState);
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        META_CONPRINTF("AdaptiveMusic Plugin - Could not pause the FMOD master bus! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    knownFMODPausedState = pausedState;
    return (0);
}

SMEXT_LINK(&g_AdaptiveMusicExt);
