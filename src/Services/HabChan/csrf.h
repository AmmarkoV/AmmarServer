#ifndef CSRF_H_INCLUDED
#define CSRF_H_INCLUDED

//Generates a fresh , unpredictable token and remembers it as currently valid , writing it ( NUL terminated ) into outToken
void generateCSRFToken(char * outToken, unsigned int outTokenSize);

//Checks a token submitted by a client against the set of currently valid , not-yet-expired tokens we handed out
int isCSRFTokenValid(const char * token);

#endif // CSRF_H_INCLUDED
