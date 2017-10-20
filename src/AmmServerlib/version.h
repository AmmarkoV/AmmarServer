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
	static const long BUILD  = 613;
	static const long REVISION  = 3114;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 776;
	#define RC_FILEVERSION 0,32,613,3114
	#define RC_FILEVERSION_STRING "0, 32, 613, 3114\0"
	static const char FULLVERSION_STRING [] = "0.32.613.3114";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 78;
	

#endif //VERSION_H
