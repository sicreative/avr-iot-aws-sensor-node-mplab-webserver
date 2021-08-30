#include "webserver.h"

#include "mcc_generated_files/debug_print.h"
#include "mcc_generated_files/winc/common/winc_defines.h"
#include <string.h>
#include "mcc_generated_files/winc/socket/socket.h"
#include "mcc_generated_files/cloud/cloud_interface.h"

#include "mcc_generated_files/cloud/cloud_service.h"
#include "mcc_generated_files/sensors_handling.h"
#include "mcc_generated_files/time_service.h"
#include <stdio.h>
#include "mcc_generated_files/winc/spi_flash/spi_flash.h"
#include "mcc_generated_files/winc/spi_flash/spi_flash_map.h"
#include "mcc_generated_files/config/mqtt_config.h" 


#define WEBSERVER_PORT      (80)
#define WEBSERVER_IP        (0xFFFFFFFF)


#define WEBSERVER_RX_DATA_SIZE 512
#define WEBSERVER_TX_DATA_SIZE 1024

#define WEBSERVER_BUFFER_SIZE 128

#define THING_NAME_FLASH_OFFSET (M2M_TLS_SERVER_FLASH_OFFSET + M2M_TLS_SERVER_FLASH_SIZE - FLASH_PAGE_SZ) 




static webserver_status_type_e status; 

static uint8_t ipAddr[4];

static SOCKET tcp_server_socket;

static SOCKET tcp_client_socket;


static char recvdata[WEBSERVER_RX_DATA_SIZE];
static uint16_t recvlength;
static char* recvdataPtr;

static char senddata[WEBSERVER_TX_DATA_SIZE];
static uint16_t sendlength;
static char* senddataPtr;

char* query = NULL;

static webserverContentPtr content_cb;

static uint32_t updateInteval = 1;



uint8_t webserver_get_key_value(char* key,char* value);

uint32_t webserver_getUpdateInterval(){
    return updateInteval;
}

char* webserver_getRecvData(){
    return recvdataPtr;
}

uint16_t webserver_content(char* body, char* path){
    
   
    
    char* timestring = TIME_GetcTime(TIME_getCurrent());
    
    char prefix[] =  "<!DOCTYPE html><html><head><style>"
                            ".button{"
                            "border: none;"
                            "color: white;"
                            "padding: 15px 32px;"
                            "text-align: center;"
                            "text-decoration: none;"
                            "display: inline-block;"
                            "font-size: 16px;"
                            "margin: 4px 2px;"
                            "cursor: pointer;"
                            "}"
                            ".buttonRefresh {background-color: #BBBBBB;}"
                            ".buttonOn {background-color: #4CAF50;}"
                            ".buttonOff {background-color: #008CBA;}"
                            ".buttonLink {background-color: #8AC9BA;}"    
                            "</style></head><body>";
      
    char suffix[] = "</body>"
                    "</html>";
                       
                   
  
    
   if(!strcmp(path,"/") || !strcmp(path,"/update")){
       
        if (!strcmp(path,"/update")){
       
        char value[50];
        uint8_t len = webserver_get_key_value("time",value);
        if (len<=0)
            return 0;
        char *ptr;
        uint32_t interval = strtoul(value,&ptr,10);
       
        if (interval>0)
            updateInteval = interval;
        

       
   } 
       
       
       
       int light = 0;
       int len = 0;    
   
   
        int16_t rawTemperature = SENSORS_getTempValue();
        light = SENSORS_getLightValue();
        

        bool isOn = updateInteval!=UINT32_MAX;
        

        len = sprintf(body,"<h2>%s Light :%d Temp %d.%02d</h2> <p> %s </p> "
                            "<p><button onclick=\"window.location.href='/'\" class=\"button buttonRefresh\">Refresh</button>"
                            "<button onclick=\"window.location.href='/update?time=%lu'\" class=\"button button%s\">%s</button></p>"
                            "<p><button onclick=\"window.location.href='https://avr-iot.com/avr-iot/aws/%s'\" class=\"button buttonlink\">AVR-IoT Development Boards</button></p>"
                            "%s",prefix,light,rawTemperature/100,abs(rawTemperature)%100,timestring,isOn?UINT32_MAX:1,isOn?"On":"Off",isOn?"Off":"On",cid,suffix);
    
    
        return len;
       
   }
        
     
    return 0;
    
}


uint8_t webserver_get_key_value(char* key,char* value){
    
    if (query==NULL)
        return 0;
    
   
   char* ptr = strstr(query,key);
   if (ptr==NULL)
       return 0;
   
   char* start = strchr(ptr,'=');
   
   if (start==NULL)
       return 0;
   
   char* end = strchr(++start,'&');
   
   if(value!=NULL){
        if (end==NULL)
            strcpy(value,start);
        else
            strncpy(value,start,end-start);
        
        value[end-start]='\0';
   }

   
   
   
   
   return end-start;

   
}







void webserver_resetClientSocket(){
     
      
      tcp_client_socket = -1;
      
      recvdataPtr = recvdata;
      
      senddataPtr = senddata;
      
      recvlength = 0;
      
      sendlength  = 0;
      
      
   
}


void webserver_init(char* attDeviceID){
   
  
      
      status = WEBSERVER_INIT;
      
      tcp_server_socket = -1;
      webserver_resetClientSocket();
      
      content_cb = webserver_content;
      
    
         
}

void webserver_start(void){
     if(WIFI_STATE_START != m2m_wifi_get_state() || status != WEBSERVER_INIT)
         return;
     
   
     
 if ( M2M_SUCCESS == m2m_wifi_get_connection_info()){
            status =  WEBSERVER_STARTING;
            
 }
     
}



void webserver_network_disconnect(){
     for(int i=0;i<4;++i)
        ipAddr[i]=0;
    
    if(tcp_server_socket!=-1){
        close(tcp_server_socket);
        tcp_server_socket=-1;
    }
    
    if(tcp_client_socket!=-1)
        webserver_resetClientSocket();
    
    status = WEBSERVER_INIT;
    
      
}

void webserver_socket_cp(SOCKET sock, uint8_t u8Msg, void *pvMsg){
    
}

void webserver_server_cb(uint8_t *data, uint8_t length){
     
}


void webserver_send_next(){
    

   
    
    //debug_printIoTAppMsg("webserver %u client msg send next: %d \r\n \r\n", tcp_client_socket,sendlength);
    if(senddataPtr>=(senddata+sendlength)){
        senddataPtr=senddata;
        return;
    }
    
    int n = senddata+sendlength-senddataPtr;
    
    if (n>WEBSERVER_BUFFER_SIZE)
        n=WEBSERVER_BUFFER_SIZE;
 
    send(tcp_client_socket, senddataPtr,n, 0);

    senddataPtr+=n;
    
  
}

void webserver_send(uint16_t len){
    
   
    if(len>WEBSERVER_TX_DATA_SIZE)
        return;

    senddataPtr = senddata;
    
    sendlength = len;
    
    
    int n = len>WEBSERVER_BUFFER_SIZE?WEBSERVER_BUFFER_SIZE:sendlength;
      
      
      
     send(tcp_client_socket, senddataPtr,n, 0);
    
   //   debug_printIoTAppMsg("webserver %u client msg send : %u %s\r\n \r\n", tcp_client_socket,n,senddataPtr);
 
      
          
     senddataPtr+=n;
      
     
  
      
     
      
}









void webserver_send_bad_request(){
    
    strcpy(senddata,"HTTP/1.1 400 Bad Request\r\n"
                    "Server: Microchip AVR-IoT\r\n"
                    "Content-Length: 49\r\n"
                    "Connection: close\r\n"
                    "Content-Type: text/html\r\n"
                    "\r\n"
                    "<html><body><h2>400 Bad Request</h2></body></html>\r\n");
    
                  
    
    uint16_t size = strlen(senddata);
      

      
    webserver_send(size);
      
}

void webserver_client_cb(uint8_t *data, uint8_t length){
    
    if (tcp_client_socket==-1)
        return;
    
    recv(tcp_client_socket, recvdataPtr,WEBSERVER_BUFFER_SIZE, 0); 
    
    char* pathptr;
    
    pathptr = strstr(recvdata,"GET");
    
    if (content_cb==NULL || pathptr==NULL ){
        
        webserver_send_bad_request();
        
        return;
    }
    
    pathptr+=4;
    
    char* pathend = strpbrk(pathptr," ?");
    
    if (pathend==NULL || pathend==pathptr){
        webserver_send_bad_request();
        return;
    }
    
    size_t pathlength = pathend-pathptr;
    char path[pathlength+1];
    strncpy(path,pathptr,pathlength);
    path[pathlength]='\0';
    
    if (*pathend=='?' && *(++pathend)!=' '){
      char* queryend = strchr(pathend,' ');
      size_t querylength = queryend-pathend;
      query = malloc((querylength+1)*sizeof(int));
      strncpy(query,pathend,querylength);
      query[querylength]='\0';
     
    }
    
    uint16_t bodylen = content_cb(recvdata,path);
    
    if (query!=NULL){
        free(query);
        query=NULL;
    }
    
    if (bodylen<=0){
        webserver_send_bad_request();
        return;
    }
   
    char head[] = "HTTP/1.1 200 OK\r\n"
                    "Server: Microchip AVR-IoT\r\n"
                    "Content-Length: %d\r\n"
                    "Connection: close\r\n"
                    "Content-Type: text/html\r\n"
                    "\r\n"
                    "%s";
      
    size_t size = strlen(head)+bodylen;
    
    sprintf(senddata,head,bodylen,recvdata);   
      
    webserver_send(size); 

}


SOCKET* webserver_getServerSocket(){
    return &tcp_server_socket;
}

SOCKET* webserver_getClientSocket(){
    
    return &tcp_client_socket;
}


void webserver_setSocketInfo(packetReceptionHandler_t* cb){
    cb[1].socket = webserver_getServerSocket();
    cb[1].recvCallBack = webserver_server_cb;
    cb[2].socket = webserver_getClientSocket();
    cb[2].recvCallBack = webserver_client_cb;
    
}

void webserver_network_connect(){
    
        struct sockaddr_in addr;
        
    addr.sin_family = AF_INET;
    addr.sin_port = BSD_htons(WEBSERVER_PORT);
    addr.sin_addr.s_addr = BSD_htonl(inet_addr((char*)ipAddr));

    
/* Open TCP server socket */
    if ((tcp_server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        debug_printIoTAppMsg("webserver: failed to create TCP server socket error!\r\n");
         }
    
/* Bind service*/
   if(SOCK_ERR_NO_ERROR != bind(tcp_server_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))){
       debug_printIoTAppMsg("webserver: failed to bind TCP server socket error!\r\n");
   }
    
    
    
}



void webserver_close(){
    webserver_network_disconnect();
}


void webserver_info_cb( tstrM2MConnInfo* pMsg){
    
    memcpy(ipAddr,pMsg->au8IPAddr,4);

    webserver_network_connect();
    status = WEBSERVER_STANDBY;
    
    
}



