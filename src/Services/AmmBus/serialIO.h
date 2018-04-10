#ifndef SERIALIO_H_INCLUDED
#define SERIALIO_H_INCLUDED


int serialport_init(const char* serialport, int baud);
int serialport_close( int fd );


int serialport_writebyte( int fd, unsigned char b);

#endif // SERIALIO_H_INCLUDED
