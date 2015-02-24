#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "24";
	static const char MONTH[] = "02";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.02";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 29;
	static const long BUILD  = 248;
	static const long REVISION  = 1238;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 242;
	#define RC_FILEVERSION 0,29,248,1238
	#define RC_FILEVERSION_STRING "0, 29, 248, 1238\0"
	static const char FULLVERSION_STRING [] = "0.29.248.1238";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 13;
	

#endif //VERSION_H
