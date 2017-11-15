#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "16";
	static const char MONTH[] = "11";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.11";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 33;
	static const long BUILD  = 646;
	static const long REVISION  = 3280;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 808;
	#define RC_FILEVERSION 0,33,646,3280
	#define RC_FILEVERSION_STRING "0, 33, 646, 3280\0"
	static const char FULLVERSION_STRING [] = "0.33.646.3280";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 11;
	

#endif //VERSION_H
