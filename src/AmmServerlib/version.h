#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "16";
	static const char MONTH[] = "06";
	static const char YEAR[] = "2013";
	static const char UBUNTU_VERSION_STYLE[] = "13.06";
	
	//Software Status
	static const char STATUS[] = "Alpha";
	static const char STATUS_SHORT[] = "a";
	
	//Standard Version Type
	static const long MAJOR = 0;
	static const long MINOR = 27;
	static const long BUILD = 84;
	static const long REVISION = 268;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 77;
	#define RC_FILEVERSION 0,27,84,268
	#define RC_FILEVERSION_STRING "0, 27, 84, 268\0"
	static const char FULLVERSION_STRING[] = "0.27.84.268";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 49;
	

#endif //VERSION_H
