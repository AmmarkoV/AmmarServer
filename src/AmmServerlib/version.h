#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "15";
	static const char MONTH[] = "07";
	static const char YEAR[] = "2014";
	static const char UBUNTU_VERSION_STYLE[] =  "14.07";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 27;
	static const long BUILD  = 116;
	static const long REVISION  = 483;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 101;
	#define RC_FILEVERSION 0,27,116,483
	#define RC_FILEVERSION_STRING "0, 27, 116, 483\0"
	static const char FULLVERSION_STRING [] = "0.27.116.483";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 81;
	

#endif //VERSION_H
