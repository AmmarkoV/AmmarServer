/** @file directory_lists.h
* @brief Basic file server functionality of AmmarServer
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef DIRECTORY_LISTS_H_INCLUDED
#define DIRECTORY_LISTS_H_INCLUDED

/**
* @brief Return a memory buffer containing the contents o a directory listing
* @ingroup filesystem
* @param System path to list
* @param Client path ( relative to root directory of client etc )
* @param Input size of memory tou allocate and Output size of memory used
* @retval Pointer to memory that contains directory listing ,0=Failure

  @bug GenerateDirectoryPage does not handle memory correctly , code is in very bad shape
*/
char * GenerateDirectoryPage(char * system_path,char * client_path,unsigned long * memoryUsed);

#endif // DIRECTORY_LISTS_H_INCLUDED
