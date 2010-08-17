#ifndef CMDPROC_H
#define CMDPROC_H

extern void commandSystemInit(void);

// return >=0: execute command successfully, return data length in cmd_buf.
// return -1: command error. the cmd should be dropped.
extern int processCMD(unsigned char *cmd_buf,int buflen);



#endif //CMDPROC_H

