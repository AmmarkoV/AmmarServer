#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "17";
	static const char MONTH[] = "03";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.03";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 29;
	static const long BUILD  = 273;
	static const long REVISION  = 1363;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 278;
	#define RC_FILEVERSION 0,29,273,1363
	#define RC_FILEVERSION_STRING "0, 29, 273, 1363\0"
	static const char FULLVERSION_STRING [] = "0.29.273.1363";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 38;
	

#endif //VERSION_H
