#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "23";
	static const char MONTH[] = "02";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.02";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 29;
	static const long BUILD  = 238;
	static const long REVISION  = 1190;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 233;
	#define RC_FILEVERSION 0,29,238,1190
	#define RC_FILEVERSION_STRING "0, 29, 238, 1190\0"
	static const char FULLVERSION_STRING [] = "0.29.238.1190";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 3;
	

#endif //VERSION_H
