#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "14";
	static const char MONTH[] = "05";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.05";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 32;
	static const long BUILD  = 554;
	static const long REVISION  = 2812;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 683;
	#define RC_FILEVERSION 0,32,554,2812
	#define RC_FILEVERSION_STRING "0, 32, 554, 2812\0"
	static const char FULLVERSION_STRING [] = "0.32.554.2812";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 19;
	

#endif //VERSION_H
