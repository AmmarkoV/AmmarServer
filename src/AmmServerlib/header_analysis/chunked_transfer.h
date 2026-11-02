/** @file chunked_transfer.h
* @brief Incremental scanner/decoder for HTTP/1.1 Transfer-Encoding: chunked request bodies
* @author Ammar Qammaz (AmmarkoV)
*/
#ifndef CHUNKED_TRANSFER_H_INCLUDED
#define CHUNKED_TRANSFER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

enum HTTPChunkedScanResult
{
   HTTP_CHUNKED_INCOMPLETE = 0, //More bytes are needed
   HTTP_CHUNKED_COMPLETE,       //Terminal chunk ( and trailers ) consumed
   HTTP_CHUNKED_ERROR_BAD,      //Malformed framing
   HTTP_CHUNKED_ERROR_TOO_BIG   //Decoded size would exceed the configured limit
};

/**
* @brief Scan a chunked body without modifying it
* @param body Start of the chunked framing ( first chunk-size line )
* @param bodyLength Bytes currently buffered
* @param maxDecodedSize Largest decoded body we are willing to accept
* @param decodedSize Output , decoded payload bytes seen so far
* @retval See enum HTTPChunkedScanResult */
enum HTTPChunkedScanResult HTTPChunkedBodyScan(
                                                const char * body,
                                                unsigned int bodyLength,
                                                unsigned long maxDecodedSize,
                                                unsigned int * decodedSize
                                              );

/**
* @brief Scan and decode a complete chunked body in place , removing chunk framing
* @param body Start of the chunked framing , writable , decoded data is copied down to the same address
* @param bodyLength Bytes buffered ( must contain a complete body )
* @param maxDecodedSize Largest decoded body we are willing to accept
* @param decodedSize Output , decoded payload size
* @retval See enum HTTPChunkedScanResult */
enum HTTPChunkedScanResult HTTPChunkedBodyDecode(
                                                 char * body,
                                                 unsigned int bodyLength,
                                                 unsigned long maxDecodedSize,
                                                 unsigned int * decodedSize
                                               );

#ifdef __cplusplus
}
#endif

#endif // CHUNKED_TRANSFER_H_INCLUDED
