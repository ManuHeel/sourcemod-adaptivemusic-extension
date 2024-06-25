#if defined _adaptivemusic_included
 #endinput
#endif
#define _adaptivemusic_included

/**
 * Load an FMOD bank
 *
 * @param bankName	The name of the bank to load. It must exist in sound/fmod/banks/ with its .strings counterpart
 * @return	The error code or 0 if no error occured
 */
native int LoadFMODBank(const char[] bankName);

/**
 * Start an FMOD event
 *
 * @param eventPath	The path of the event to start. It must exist in sound/fmod/banks/ with its .strings counterpart
 * @return	The error code or 0 if no error occured
 */
native int StartFMODEvent(const char[] eventPath);

/**
 * Stop an FMOD event
 *
 * @param eventPath	The path of the event to stop. It must exist in sound/fmod/banks/ with its .strings counterpart
 * @return	The error code or 0 if no error occured
 */
native int StopFMODEvent(const char[] eventPath);

/**
 * Set an FMOD global parameter value
 *
 * @param parameterName	The path of the global parameter to set
 * @param value The value to set the global parameter to
 * @return	The error code or 0 if no error occured
 */
native int SetFMODGlobalParameter(const char[] parameterName, float value);

/**
 * Set if the FMOD engine should be paused or not
 *
 * @param pausedState 1 if the engine should be paused, 0 if not
 * @return	The error code or 0 if no error occured
 */
native int SetFMODPausedState(int pausedState);