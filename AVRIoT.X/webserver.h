/* 
 * File:   webserver.h
 * Author: slee
 *
 * Created on August 25, 2021, 10:03 PM
 */

#ifndef WEBSERVER_H
#define	WEBSERVER_H

#include "mcc_generated_files/winc/m2m/m2m_wifi.h"
#include "mcc_generated_files/mqtt/mqtt_winc_adapter.h"

/**
 * Types of webserver condition.
 */
typedef enum webserver_status_type_e
{
    WEBSERVER_INIT,  
    WEBSERVER_STARTING,
    WEBSERVER_STANDBY        
} webserver_status_type_e;


typedef uint16_t (*webserverContentPtr)(char* body, char* path);


void webserver_init(char* attDeviceID);

void webserver_network_connect();

void webserver_start();

void webserver_close();

void webserver_info_cb(tstrM2MConnInfo* pMsg);

void webserver_setSocketInfo(packetReceptionHandler_t* cb);

SOCKET* webserver_getServerSocket();

SOCKET* webserver_getClientSocket();

void webserver_resetClientSocket();



void webserver_send_next();

void webserver_send(uint16_t len);

uint32_t webserver_getUpdateInterval();

char* webserver_getRecvData();

#endif	/* WEBSERVER_H */

