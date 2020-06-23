# Protocolos de Comunicacion - TPE

La versión funcional del servidor y client SCTP se encuentran en la rama **develop**.

## Códigos Fuente

### Servidor

El código fuente del servidor se encuentra en dos carpetas, en la carpeta **/src** se encuentran todos los archivos .c, mientras que en la carpeta **/include** se encuentran los archivos header correspondientes para esos archivos .c.

### Cliente SCTP

El código fuente del cliente SCTP se encuentra en la carpeta **/management_client**, en esa carpeta están los archivos .c y los archivos header correspondientes para esos archivos .c.

### Datos Persistentes

El servidor posee unas credenciales persistentes default para los administradores y usuarios del proxy, estas credenciales se encuentran en la carpeta **/serverdata**. Los usuarios administradores se encuentran en el archivo "*admin_user_pass.txt*" y los usuarios del proxy en el archivo "*user_pass.txt*".

El formato de los archivos es el siguiente "Username Password", separados por un espacio y cada linea indica un nuevo par de credenciales. No seguir este formato puede llevar al mal funcionamiento del servidor.

## Compilación e Instalación

Para poder compilar es necesario una librería no estándar de SCTP, que no se encuentra disponible en OSx.

Para descargar esta librería en una distribución de linux se corre el siguiente comando (en el caso que no se use apt-get, utilizar el equivalente en su propia distribución):
```
$ sudo apt-get install libsctp-dev
```

Para compilar el programa simplemente se puede ejecutar:

```
$ make clean all
```

Este comando va compilar la aplicación servidor y el cliente SCTP

## Configuración

El servidor y el cliente SCTP poseen un *manfile* cada uno dentro de la carpeta de documentación (/Documentacion), y se pueden acceder de esta forma:

```
$ man <path_al_archivo>
```

Aquí se pueden ver las configuraciones disponibles para cada una de las aplicaciones que se construyen

## Ejecución

Para ejecutar el sevidor se corre el comando 

```
$ ./server <Opciones>
```

Para ejecutar el cliente SCTP se corre el comando

```
$ ./sctp_client <Opciones>
```


Las opciones de cada ejecutable pueden verse haciendo 

```
$ ./server -h
```

o

```
$ ./sctp_client -h
```

## Documentación

El informe se encuentra en la carpeta **/Documentacion** bajo el nombre "*pc-2020a-5-informe.pdf*".

Los manfiles conteniendo la información sobre los parámetros que reciben el servidor y el client SCTP se encuentran también en la carpeta **/Documentacion**, con los nombres "*socks5d.8*" y "*sctp_clientd.8*".

Estos archivos pueden verse con el siguiente comando:
```
$ man <path_al_archivo>
```

Se incluye también una copia del protocolo desarrollado por nosotros en formato .txt, el mismo se encuentra en la carpeta **/Documentacion** bajo el nombre "*Protocolo_IGAF.txt*".

## Tests

En la carpeta de **/tests** se incluyen algunos scripts y archivos de C que se usaron para realizar diversas pruebas, estos archivos y su uso son los siguientes:
 - test.sh -> Script para realizar un test de N descargas concurrentes de diferentes tamaños usando curl
    
    Su uso es el siguiente:
    ```
    $ ./test.sh [-xs,-s,-m,-l] [number_of_connecions] [outputFilePath]
    ```
 - testDns.sh -> Script para realizar un test de 300 conexiones concurrentes a diferentes dominios haciendo resolución de DNS usando curl
    
    Su uso es el siguiente:
    ```
    $ ./testDns.sh
    ```
 - test_time.sh -> Script para realizar un test de comparación de tiempos de descarga con y sin el servidor proxy, los resultados de los tiempos se dejan en un archivo de texto.
    
    Su uso es el siguiente:
    ```
    $ ./test_time.sh [-xs,-s,-m,-l] [-4, -6, -name] [amount] [outputFilePath]
    ```
 - /parsers -> En este directorio se encuentran archivos de test para algunos de los parsers desarrollados, cada una de las carpetas del parser contiene su propio makefile para poder compilarlo, y para poder ejecutarlo hay que ejecutar el archivo binario que genera el makefile en cada caso

## Autores

**Grupo 5**

Gonzalo Hirsch - ghirsch@itba.edu.ar

Augusto Henestrosa - ahenestrosa@itba.edu.ar

Ignacio Ribas - iribas@itba.edu.ar

Francisco Choi - fchoi@itba.edu.ar

## Versión

Versión 1.0

TPE para Protocolos de Comunicacion

1er Cuatrimestre - 2020