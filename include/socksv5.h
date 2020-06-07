#ifndef __SOCKSV5_H__
#define __SOCKSV5_H__

typedef enum AuthType {
    NO_AUTH = 0x00,
    USER_PASSWORD = 0x02
} AuthType;

typedef enum AddrType {
    IPv4 = 0x01,
    DOMAIN_NAME = 0x03,
    IPv6 = 0x04
} AddrType;

typedef enum ConnType {
    TCP_IP_STREAM = 0x01,
    TCP_IP_PORT_BINDING = 0x02,
    UDP = 0x03
} ConnType;



#endif