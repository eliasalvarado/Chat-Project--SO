# Chat-Project--SO

 ## Autores
 - Adrian Fulladolsa - 21592
 - Elías Alvarado - 21808

Este proyecto es un servidor de chat que permite a múltiples usuarios conectarse, registrarse, actualizar su estado y enviar mensajes entre ellos. El servidor está construido utilizando sockets en C++ y hace uso de Google Protocol Buffers.

## Funcionalidades

- **Registro de usuarios**: Los usuarios pueden registrarse con un nombre de usuario único.
- **Actualización de estado**: Los usuarios pueden actualizar su estado (en línea, fuera de línea, etc.).
- **Lista de usuarios**: Los usuarios pueden obtener la lista de usuarios en línea.
- **Mensajes**: Los usuarios pueden enviar mensajes directos a otros usuarios o mensajes de difusión a todos los usuarios en línea.
- **Monitor de inactividad**: El servidor automáticamente establece a los usuarios como fuera de línea si están inactivos durante un período de tiempo configurable.

## Requisitos

- **C++** 11 o superior
- **Google Protocol Buffers** (protoc, libprotobuf)
- **pthread** (biblioteca de hilos POSIX)

## Compilación y ejecución

1. **Compilación**:
   
   Dentro del directorio donde se aloja este repositorio ejecutar el comando:
   ```
   make
   ```
   Esto compilará los archivos ```server.cpp```, ```client.cpp``` y ```chat.proto``` con sus librerías respectivas.
2. **Ejecución**:
   Una vez tengamos compilados los programar, procedemos a montar el servidor con el siguiente comando:
   ```
   ./server <port>
   ```
   Donde \<port\> debe de ser reemplazado por el número de puerto que se desee abrir, por ejemplo:
   ```
   ./server 8080
   ```
   Al estar seguros de que el servidor está escuchando en el puerto \<port\> se puede proceder a conectar los clientes deseados con el comando:
   ```
   ./client <username> <serverIP> <port>
   ```
   Donde \<username\> será el nombre de usuario que se desee tomar, \<serverIP\> es la IP donde está alojado nuestro servidor y \<port\> será el puerto en donde nuestro servidor está escuchando.
