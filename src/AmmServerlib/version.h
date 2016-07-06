#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "06";
	static const char MONTH[] = "07";
	static const char YEAR[] = "2016";
	static const char UBUNTU_VERSION_STYLE[] =  "16.07";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 31;
	static const long BUILD  = 517;
	static const long REVISION  = 2620;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 637;
	#define RC_FILEVERSION 0,31,517,2620
	#define RC_FILEVERSION_STRING "0, 31, 517, 2620\0"
	static const char FULLVERSION_STRING [] = "0.31.517.2620";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 82;
	

#endif //VERSION_H
