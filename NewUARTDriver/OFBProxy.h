/* 
 * File:   OFBProxy.h
 * Author: jesseharkin
 *
 * Created on February 21, 2013, 1:23 PM
 */

#ifndef OFBPROXY_H
#define	OFBPROXY_H

void OFB_set(unsigned char *buffer);
unsigned int OFB_getSize (void);
int OFB_read_tail(unsigned char outputarray[]);
int OFB_pop(void);

#endif	/* OFBPROXY_H */

