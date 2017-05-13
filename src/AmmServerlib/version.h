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
	static const long BUILD  = 551;
	static const long REVISION  = 2801;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 680;
	#define RC_FILEVERSION 0,32,551,2801
	#define RC_FILEVERSION_STRING "0, 32, 551, 2801\0"
	static const char FULLVERSION_STRING [] = "0.32.551.2801";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 16;
	

#endif //VERSION_H
