# Protocolos de Comunicacion - TPE

## Compilación e Instalación

Primeramente se debe tener:

export DOH='--doh-port 80 --doh-path /dns-query --doh-host doh'
sudo apt-get install libsctp-dev

Para compilar el programa simplemente se puede ejecutar:

```
$ make clean all
```

Este comando va compilar la aplicación servidor y el clienteSCTP

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

## Informe

El informe se encuentra en la carpeta 'Documentacion', con el nombre de 'pc-2020a-5-informe.pdf'

## Autores

Gonzalo Hirsch

Augusto Henestrosa

Ignacio Ribas

Francisco Choi

## Testing

Links útiles para testing con JMeter: https://jmeter.apache.org/usermanual/jmeter_proxy_step_by_step.pdf

## Versión

Versión 1.0

TPE para Protocolos de Comunicacion

2020

1er Cuatrimestre