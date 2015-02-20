#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "20";
	static const char MONTH[] = "02";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.02";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 29;
	static const long BUILD  = 237;
	static const long REVISION  = 1183;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 233;
	#define RC_FILEVERSION 0,29,237,1183
	#define RC_FILEVERSION_STRING "0, 29, 237, 1183\0"
	static const char FULLVERSION_STRING [] = "0.29.237.1183";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 2;
	

#endif //VERSION_H
