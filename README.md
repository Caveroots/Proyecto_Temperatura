# Proyecto_Temperatura

El proyecto consiste en poder medir temperatura en diferentes puntos y poder acceder a la data a través de internet. 
Cuenta con 4 nodos que sensan temperatura con el sensor DS18B20 y envian la data por radiofrecuencia con el nRF24L01 a un modulo principal, el cual recepciona la data y la sube a internet a través del módulo wi-fi esp01. Finalmente, se visualizan los datos por Thingspeak, dashboard desarrollado por Matlab.
Fue desarrollado en el microcontrolador Atmega328p de Atmel.
