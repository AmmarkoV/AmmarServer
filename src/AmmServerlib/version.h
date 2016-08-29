#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "26";
	static const char MONTH[] = "08";
	static const char YEAR[] = "2016";
	static const char UBUNTU_VERSION_STYLE[] =  "16.08";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 31;
	static const long BUILD  = 533;
	static const long REVISION  = 2698;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 661;
	#define RC_FILEVERSION 0,31,533,2698
	#define RC_FILEVERSION_STRING "0, 31, 533, 2698\0"
	static const char FULLVERSION_STRING [] = "0.31.533.2698";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 98;
	

#endif //VERSION_H
