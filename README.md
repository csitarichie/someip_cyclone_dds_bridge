# someip_cyclone_dds_bridge
Brain dump on how to implement a some ip eclipse iceoryx bride

## The Task
Create a gateway concept for a bridge between someip and iceoryx.

## What is SomeIP
Scalable service-Oriented MiddlewarE over IP (SOME/IP)

SOME/IP is an automotive middleware solution that can be used for control messages. It was designed from beginning on to fit devices of different sizes and different operating systems perfectly.

Standard defined by AUTOSAR.

* SOME/IP Service Discovery Protocol Specification [link](https://www.autosar.org/fileadmin/user_upload/standards/foundation/20-11/AUTOSAR_PRS_SOMEIPServiceDiscoveryProtocol.pdf)
* SOME/IP Protocol Specification [link](https://www.autosar.org/fileadmin/user_upload/standards/foundation/20-11/AUTOSAR_PRS_SOMEIPProtocol.pdf)

SOME/IP SD (Service Discovery) is a protocol implement control plane functions for SOME/IP IPC
SOME/IP Protocol itself defines the data serialisation and packet headers and communication types. 

SIME/IP SD uses SOME/IP to implement message encoding / decoding.
SOME/IP SD implements multicast discovery similar to bonjour.
A SOME/IP implementation can be found here: [link]/(https://github.com/COVESA/vsomeip)]

## What is iceoryx
iceoryx - true zero-copy inter-process-communication

iceoryx implements event bus type communication publish subscribe mechanism.

iceoryx documentation available here [link](https://iceoryx.io/)

## Initial Idea

Create my own message dispatcher framework.
Which connects the user facing APIS from. iceoryx and vSOME/IP.
Initial try to map iceoryx publish to SOME/IP offer with notify then SOME/IP subscribe notify to iceoryx subscribe.

SOME/IP implements the following ICP models:
* Request/Response Methods: A request sent from a client to the server and a response sent back from server to client.
Additionally, SOME/IP allows error responses to be sent back from server to client instead of the regular response. This feature might be used to implement a different payload format in error cases.
* Fire and Forget Methods: A request is sent from a client to the server.
* Event: An event is sent from the server to a relevant clients. Which client needs this event will be determined via 
  SOME/IP-SD.
* Field: A field can have an option notifier (event to be sent cyclically or on-change), an optional setter (a 
  request/response method to update the field), and an optional getter (a request/response method to read out the current value of the field.)

Iceoryx supports the following ICP models:

* Event: Publish / subscribe.

since iceoryx supports event based communication only an mapping is need to offer RPC like communication from SOME/IP
to iceoryx

Following mapping can be made:

* SOME/IP Request/Response Methods can be mapped to 2 events -> 1 request from client to server and to one response 
  server to client
* SOME/IP Fire and Forget Methods can be mapped to a simple Indication event from client to server.
* SOME/IP Event is mapped to one to iceoryx event
* SOME/IP Field is a most complex to implement since here 5 event is needed to implement the complete SOME/IP 
  functionality:
  * Request / Response even pair to implement set methode
  * Request / Response even pair to implement get methode
  * and one subscribe event for notify on change

## Very First implementation
 * ToDo Build vSomeIP and iceoryx in a single project.
 * ToDo Create description UML models Instance Models and message sequence charts for the initial event implementation.
 * ToDo Create event dispatchers and actors to implement scalable gateway elements.
 * ToDo Connect API calls from client facing APIs
 * If time allows, add testing framework
 * if time allows, add implementation of extended SOME/IP services.

## Getting Stared
How to use the source code and the build system. 

### Building
As of now build is working on OSX/Darwin with prerequisites installed by Darwin.

Prerequisites:
* cmake `brew install cmake` version is 3.22
* ninja `brew install ninja` version 1.10.2
* bison `brew install bison` version 3.8.2
* git   `brew install git`   version 2.34.1
* ToDo Add vSomeIP dependencies

### Dispatcher Concept

### Dispatcher UML

### Dispatcher sequence diagram

