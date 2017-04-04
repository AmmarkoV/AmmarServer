#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "04";
	static const char MONTH[] = "04";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.04";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 32;
	static const long BUILD  = 537;
	static const long REVISION  = 2731;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 666;
	#define RC_FILEVERSION 0,32,537,2731
	#define RC_FILEVERSION_STRING "0, 32, 537, 2731\0"
	static const char FULLVERSION_STRING [] = "0.32.537.2731";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 2;
	

#endif //VERSION_H
