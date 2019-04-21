# ESP8266ATLib-for-Spresense
ESP8266ATLib for Spresense 

This library is made for Spresense Wi-Fi Add-on board. However you can use this library with ESP8266 module connected to Spresense UART0. 
You can get the Wi-Fi Add-on board at the online site of Switch Science (https://www.switch-science.com/catalog/4042/)

This library has three examples.

## (1) Spresense ESP8266ATLib Simple Clinet<br/>
This example is a Arduino sketch of a TCP client. 
"herclus" that is a test TCP server application for Windows is very convenient for this sample. 
The download site is here "https://www.hw-group.com/software/hercules-setup-utility"

## (2) Spresense ESP8266ATLib Simple Server<br/>
This example is a Arduino sketch of a simple HTTP server. When you access to a URL indicated at the console, 
you will see a "Hello Client" message on your web browser.

## (3) Spresense ESP8266ATLib Simple Camera Server<br/>
This example is a Arduino sketch of a simple Camera server. 
This example needs Spresense Camera board that is sold at Switch Science as well.
(https://www.switch-science.com/catalog/4119/)
The combination of Spresense main board, camera board and Wi-Fi Add-on board is very small and super low power, 
so it is able to be driven by dry cell batteries.
When you access to a URL idecated at the console, you will see a picture taken by Spresense camera board.
And you try a html file in the resource directly, 
the spresense that this sketch is installed in will turn into a Web Camera server changing the picture every 5 seconds automatically.





