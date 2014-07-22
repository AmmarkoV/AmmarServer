#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "22";
	static const char MONTH[] = "07";
	static const char YEAR[] = "2014";
	static const char UBUNTU_VERSION_STYLE[] =  "14.07";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 27;
	static const long BUILD  = 131;
	static const long REVISION  = 580;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 113;
	#define RC_FILEVERSION 0,27,131,580
	#define RC_FILEVERSION_STRING "0, 27, 131, 580\0"
	static const char FULLVERSION_STRING [] = "0.27.131.580";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 96;
	

#endif //VERSION_H
