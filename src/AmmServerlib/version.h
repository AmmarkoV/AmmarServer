#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "12";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.09";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 30;
	static const long BUILD  = 337;
	static const long REVISION  = 1692;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 361;
	#define RC_FILEVERSION 0,30,337,1692
	#define RC_FILEVERSION_STRING "0, 30, 337, 1692\0"
	static const char FULLVERSION_STRING [] = "0.30.337.1692";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 2;
	

#endif //VERSION_H
