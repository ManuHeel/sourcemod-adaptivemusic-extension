/**
 * Load an FMOD bank
 *
 * @param bankName	The name of the bank to load. It must exist in sound/fmod/banks/ with its .strings counterpart
 * @return	The error code or 0 if no error occured
 */
native int LoadFMODBank(const char[] bankName);