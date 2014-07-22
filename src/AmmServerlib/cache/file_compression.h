/** @file file_compression.h
* @brief A tool that compresses memory blocks for better bandwidth usage on the expense of computing power
* @author Ammar Qammaz (AmmarkoV)
* @bug Compression should be improved
*/

#ifndef FILE_COMPRESSION_H_INCLUDED
#define FILE_COMPRESSION_H_INCLUDED



/**
* @brief Create compressed version of dynamic content , this should be used via the shortcut functions that control compression levels automatically
* @ingroup compression
* @param An AmmarServer Instance
* @param Index of cache item
* @param Compression level 1-9
* @retval 1=Success,0=Failure*/
inline int CreateCompressedVersionofCachedResource(struct AmmServer_Instance * instance,unsigned int index,int compression_level);



/**
* @brief Create compressed version of dynamic content , cache item
* @ingroup compression
* @param An AmmarServer Instance
* @param Index of cache item
* @retval 1=Success,0=Failure*/
int CreateCompressedVersionofDynamicContent(struct AmmServer_Instance * instance,unsigned int index);

/**
* @brief Create compressed version of static content , cache item
* @ingroup compression
* @param An AmmarServer Instance
* @param Index of cache item
* @retval 1=Success,0=Failure*/
int CreateCompressedVersionofStaticContent(struct AmmServer_Instance * instance,unsigned int index);

/**
* @brief Create compressed version of static content which is preloaded , cache item
* @ingroup compression
* @param An AmmarServer Instance
* @param Index of cache item
* @retval 1=Success,0=Failure*/
int CreateCompressedVersionofStaticContentPreloading(struct AmmServer_Instance * instance,unsigned int index);

#endif // FILE_COMPRESSION_H_INCLUDED
