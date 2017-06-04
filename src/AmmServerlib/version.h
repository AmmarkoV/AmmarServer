#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "04";
	static const char MONTH[] = "06";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.06";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 32;
	static const long BUILD  = 585;
	static const long REVISION  = 2978;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 736;
	#define RC_FILEVERSION 0,32,585,2978
	#define RC_FILEVERSION_STRING "0, 32, 585, 2978\0"
	static const char FULLVERSION_STRING [] = "0.32.585.2978";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 50;
	

#endif //VERSION_H
