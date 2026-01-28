# AESD Final Project Apps
This repository contains the 3 applications used in the project.

For more information about the project, see the [Wiki](https://github.com/flopezb03/aesd-final-project-apps/wiki).

---
Este repositorio contiene las 3 aplicaciones utilizadas en el proyecto. 

Para más información del proyecto, consulta la [Wiki](https://github.com/flopezb03/aesd-final-project-apps/wiki).
## telemetry-app
This application is responsible for interacting with the hardware: BMP280 temperature and pressure sensor, HD44780U LCD display, and RGB LED.

The application first initializes communication with the peripherals and prepares unix  socket communication with the telemetry-client application. After that, it enters a loop where it reads data from the sensor, writes it to the LCD, and sends it through the unix socket.

The application also has a thread listening on the socket (asynchronous communication) and changes the LED color according to the received message.

This application is installed on the device (Raspberry Pi 3B+).

---
Esta aplicación se encarga de la interactuación con el hardware: sensor de temperatura y presión BMP280, pantalla LCD HD44780U y LED RGB.

La aplicación primero incializará la comunicación con los periféricos y preparación de la comunicación por socket unix con la aplicación telemetry-app. Tras ello entrará en un bucle en el que leerá datos del sensor, los escribirá en la LCD y los enviará por el socket unix. 

La aplicación también tiene un thread escuchando en el socket (comunicación asíncrona) y cambiará el color del LED según el mensaje recibido.

Esta aplicación se instala en el dispositivo (Raspberry Pi 3B+).
## telemetry-client
This application acts as an intermediary between telemetry-app and the server.

It is responsible for creating the unix socket used to communicate with telemetry-app, which runs as a client. It also connects to the server as a client using a TCP socket. It then forwards the data received from telemetry-app to the server.

In another thread, it listens to the TCP socket (asynchronous communication) and forwards the received data to telemetry-app.

The application is resilient to potential server connection losses and will continue running and attempting to reconnect.

This application is installed on the device (Raspberry Pi 3B+).

---
Esta aplicación sirve de intermediario entre telemetry-app y el servidor. 

Se encarga de crear el socket unix con el que se comunicará con telemetry-app que funciona como cliente. También se conecta como cliente al servidor mediante un socket TCP. Después Reenviará los datos recibidos de telemetry-app al servidor. 

En otro thread, escuchará al socket TCP (comunicación asíncrona) reenviando los datos recibidos a telemetry-app.

La aplicación es resistente a posibles perdidas de conexión con el servidor y continuará funcionando e intentando reconectarse.

Esta aplicación se instala en el dispositivo (Raspberry Pi 3B+).

## server-app
This application creates a TCP socket to which telemetry-client connects as a client. In a loop, it reads the data forwarded by telemetry-client and prints them to the console. Each received data item is "processed", and an instruction can be sent through the TCP socket.

This application is NOT installed on the device.

---
Esta aplicación crea un socket TCP al que telemetry-client se conectará como cliente. En bucle lee los datos que telemetry-client le reenvía y los muestra por consola. Cada dato recibido es "procesado" y se puede enviar una instrucción por el socket TCP para que telemetry-app la ejecute.

Esta aplicación NO se instala en el dispositivo.
