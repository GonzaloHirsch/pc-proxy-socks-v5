#ifndef __NETUTILS_H__
#define __NETUTILS_H__

//------------------------SACADO DE EJEMPLO PARCIAL---------------------------
// https://campus.itba.edu.ar/webapps/blackboard/execute/content/file?cmd=view&content_id=_238320_1&course_id=_12752_1&framesetWrapped=true

#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // memset
#include <assert.h> // assert
#include <errno.h>
#include <time.h>
#include <unistd.h> // close
#include <pthread.h>

#include <arpa/inet.h>

#include "hello.h"
#include "request.h"
#include "buffer.h"

#include "stateMachine.h"
#include "netutils.h"

#define N(x) (sizeof(x)/sizeof((x)[0]))

#endif