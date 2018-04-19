#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "20";
	static const char MONTH[] = "04";
	static const char YEAR[] = "2018";
	static const char UBUNTU_VERSION_STYLE[] =  "18.04";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 35;
	static const long BUILD  = 925;
	static const long REVISION  = 4750;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 1094;
	#define RC_FILEVERSION 0,35,925,4750
	#define RC_FILEVERSION_STRING "0, 35, 925, 4750\0"
	static const char FULLVERSION_STRING [] = "0.35.925.4750";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 90;
	

#endif //VERSION_H
