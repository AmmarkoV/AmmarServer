#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "13";
	static const char MONTH[] = "03";
	static const char YEAR[] = "2018";
	static const char UBUNTU_VERSION_STYLE[] =  "18.03";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 0;
	static const long MINOR  = 35;
	static const long BUILD  = 886;
	static const long REVISION  = 4534;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 1058;
	#define RC_FILEVERSION 0,35,886,4534
	#define RC_FILEVERSION_STRING "0, 35, 886, 4534\0"
	static const char FULLVERSION_STRING [] = "0.35.886.4534";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 51;
	

#endif //VERSION_H
