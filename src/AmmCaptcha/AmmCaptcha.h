#ifndef AMMCAPTCHA_H_INCLUDED
#define AMMCAPTCHA_H_INCLUDED

int AmmCaptcha_initialize(char * font,char * dictFilename);
int AmmCaptcha_destroy();
int AmmCaptcha_isReplyCorrect(unsigned int captchaID, char * reply);
int AmmCaptcha_getCaptchaFrame(unsigned int captchaID, char *mem,unsigned long * mem_size);
int AmmCaptcha_getJPEGFileFromPixels(char * pixels , unsigned int width , unsigned int height , unsigned int channels , char *mem, unsigned long * mem_size);

int testAmmCaptcha();
#endif // AMMCAPTCHA_H_INCLUDED
