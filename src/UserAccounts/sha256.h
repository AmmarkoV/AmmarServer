/** @file sha256.h
* @brief Small, self-contained SHA-256 implementation ( FIPS 180-4 ) - used to hash passwords in userAccounts.c.
*        Deliberately not gated behind the optional, off-by-default USE_OPENSSL flag ( see top-level CMakeLists.txt ) :
*        account security needs to work in the default build, not only when TLS happens to be enabled too.
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef SHA256_H_INCLUDED
#define SHA256_H_INCLUDED

#define SHA256_DIGEST_SIZE 32

struct SHA256_Context
{
  unsigned char data[64];
  unsigned int datalen;
  unsigned long long bitlen;
  unsigned int state[8];
};

void SHA256_Init(struct SHA256_Context * ctx);
void SHA256_Update(struct SHA256_Context * ctx,const unsigned char * data,unsigned int len);
void SHA256_Final(struct SHA256_Context * ctx,unsigned char outDigest[SHA256_DIGEST_SIZE]);

/** @brief One-shot convenience wrapper around Init/Update/Final */
void SHA256_Hash(const unsigned char * data,unsigned int len,unsigned char outDigest[SHA256_DIGEST_SIZE]);

/** @brief Hex-encodes a digest ( or any byte buffer ) into outHex, which must be at least len*2+1 bytes */
void SHA256_ToHex(const unsigned char * data,unsigned int len,char * outHex,unsigned int outHexSize);

#endif // SHA256_H_INCLUDED
