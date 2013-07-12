#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "12";
	static const char MONTH[] = "07";
	static const char YEAR[] = "2013";
	static const char UBUNTU_VERSION_STYLE[] = "13.07";
	
	//Software Status
	static const char STATUS[] = "Alpha";
	static const char STATUS_SHORT[] = "a";
	
	//Standard Version Type
	static const long MAJOR = 0;
	static const long MINOR = 27;
	static const long BUILD = 97;
	static const long REVISION = 362;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 85;
	#define RC_FILEVERSION 0,27,97,362
	#define RC_FILEVERSION_STRING "0, 27, 97, 362\0"
	static const char FULLVERSION_STRING[] = "0.27.97.362";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 62;
	

#endif //VERSION_H
