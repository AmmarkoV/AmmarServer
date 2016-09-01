#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "01";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2016";
	static const char UBUNTU_VERSION_STYLE[] =  "16.09";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 31;
	static const long BUILD  = 534;
	static const long REVISION  = 2706;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 662;
	#define RC_FILEVERSION 0,31,534,2706
	#define RC_FILEVERSION_STRING "0, 31, 534, 2706\0"
	static const char FULLVERSION_STRING [] = "0.31.534.2706";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 99;
	

#endif //VERSION_H
