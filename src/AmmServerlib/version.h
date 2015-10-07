#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "07";
	static const char MONTH[] = "10";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.10";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 31;
	static const long BUILD  = 436;
	static const long REVISION  = 2178;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 484;
	#define RC_FILEVERSION 0,31,436,2178
	#define RC_FILEVERSION_STRING "0, 31, 436, 2178\0"
	static const char FULLVERSION_STRING [] = "0.31.436.2178";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 1;
	

#endif //VERSION_H
