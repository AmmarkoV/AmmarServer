#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "31";
	static const char MONTH[] = "10";
	static const char YEAR[] = "2012";
	static const char UBUNTU_VERSION_STYLE[] = "12.10";
	
	//Software Status
	static const char STATUS[] = "Alpha";
	static const char STATUS_SHORT[] = "a";
	
	//Standard Version Type
	static const long MAJOR = 0;
	static const long MINOR = 27;
	static const long BUILD = 40;
	static const long REVISION = 32;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 21;
	#define RC_FILEVERSION 0,27,40,32
	#define RC_FILEVERSION_STRING "0, 27, 40, 32\0"
	static const char FULLVERSION_STRING[] = "0.27.40.32";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 5;
	

#endif //VERSION_H
