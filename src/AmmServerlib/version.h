#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "25";
	static const char MONTH[] = "08";
	static const char YEAR[] = "2016";
	static const char UBUNTU_VERSION_STYLE[] =  "16.08";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 31;
	static const long BUILD  = 532;
	static const long REVISION  = 2690;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 657;
	#define RC_FILEVERSION 0,31,532,2690
	#define RC_FILEVERSION_STRING "0, 31, 532, 2690\0"
	static const char FULLVERSION_STRING [] = "0.31.532.2690";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 97;
	

#endif //VERSION_H
