#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "20";
	static const char MONTH[] = "10";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.10";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 32;
	static const long BUILD  = 616;
	static const long REVISION  = 3131;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 779;
	#define RC_FILEVERSION 0,32,616,3131
	#define RC_FILEVERSION_STRING "0, 32, 616, 3131\0"
	static const char FULLVERSION_STRING [] = "0.32.616.3131";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 81;
	

#endif //VERSION_H
