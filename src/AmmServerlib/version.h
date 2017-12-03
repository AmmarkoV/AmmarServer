#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "03";
	static const char MONTH[] = "12";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.12";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 33;
	static const long BUILD  = 648;
	static const long REVISION  = 3285;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 809;
	#define RC_FILEVERSION 0,33,648,3285
	#define RC_FILEVERSION_STRING "0, 33, 648, 3285\0"
	static const char FULLVERSION_STRING [] = "0.33.648.3285";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 13;
	

#endif //VERSION_H
