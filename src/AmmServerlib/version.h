#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "09";
	static const char MONTH[] = "10";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.10";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 31;
	static const long BUILD  = 480;
	static const long REVISION  = 2442;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 584;
	#define RC_FILEVERSION 0,31,480,2442
	#define RC_FILEVERSION_STRING "0, 31, 480, 2442\0"
	static const char FULLVERSION_STRING [] = "0.31.480.2442";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 45;
	

#endif //VERSION_H
