#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "04";
	static const char MONTH[] = "04";
	static const char YEAR[] = "2016";
	static const char UBUNTU_VERSION_STYLE[] =  "16.04";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 31;
	static const long BUILD  = 512;
	static const long REVISION  = 2608;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 632;
	#define RC_FILEVERSION 0,31,512,2608
	#define RC_FILEVERSION_STRING "0, 31, 512, 2608\0"
	static const char FULLVERSION_STRING [] = "0.31.512.2608";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 77;
	

#endif //VERSION_H
