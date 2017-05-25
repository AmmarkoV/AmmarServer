#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "25";
	static const char MONTH[] = "05";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.05";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 32;
	static const long BUILD  = 570;
	static const long REVISION  = 2903;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 701;
	#define RC_FILEVERSION 0,32,570,2903
	#define RC_FILEVERSION_STRING "0, 32, 570, 2903\0"
	static const char FULLVERSION_STRING [] = "0.32.570.2903";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 35;
	

#endif //VERSION_H
