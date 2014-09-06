#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "06";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2014";
	static const char UBUNTU_VERSION_STYLE[] =  "14.09";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 28;
	static const long BUILD  = 203;
	static const long REVISION  = 998;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 195;
	#define RC_FILEVERSION 0,28,203,998
	#define RC_FILEVERSION_STRING "0, 28, 203, 998\0"
	static const char FULLVERSION_STRING [] = "0.28.203.998";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 68;
	

#endif //VERSION_H
