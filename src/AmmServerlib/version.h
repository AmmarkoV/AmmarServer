#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "04";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.09";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 29;
	static const long BUILD  = 323;
	static const long REVISION  = 1617;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 347;
	#define RC_FILEVERSION 0,29,323,1617
	#define RC_FILEVERSION_STRING "0, 29, 323, 1617\0"
	static const char FULLVERSION_STRING [] = "0.29.323.1617";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 88;
	

#endif //VERSION_H
