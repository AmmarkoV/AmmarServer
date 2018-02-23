#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "23";
	static const char MONTH[] = "02";
	static const char YEAR[] = "2018";
	static const char UBUNTU_VERSION_STYLE[] =  "18.02";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 33;
	static const long BUILD  = 706;
	static const long REVISION  = 3570;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 864;
	#define RC_FILEVERSION 0,33,706,3570
	#define RC_FILEVERSION_STRING "0, 33, 706, 3570\0"
	static const char FULLVERSION_STRING [] = "0.33.706.3570";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 71;
	

#endif //VERSION_H
