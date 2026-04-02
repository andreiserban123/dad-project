Problem description:
Create 5+ Docker containers from Docker Image (eventually using 'docker build' command):
https://hub.docker.com/r/critoma/amd64_u24_noble_ism_security

for Encrypting and Decrypting with symmetric AES key a large BMP picture.

A. Diagram Request
Please draw the architecture in draw.io/diagrams.net/Visio/ or equivalent tools and implement the following requirements.

B. Implementation Request
The picture and the symmetric key is received from the Front-end (Plain JS/CSS/HTML or Angular or Vue.js or REACT or ASP.NET) into the 1st container Back-End via REST API (Javalin / Jakarta EE Servlet REST API (JAX-RS) / Java Spring Boot or node.js (express) / Nest or C# .NET) and then is published as binary message into a JMS/RabbitMQ Topic from the 2nd container which is a MoM JMS/RabbitMQ Broker.
In the 3rd container a Jakarta EE EJB MDB/POJO Java Client Subscriber is subscribed to the 2nd container JMS Topic and it is also launching a native process (either JNI/JNA or Runtime class) from an native exe/ELF64 which is an OpenMPI&OpenMP client for picture distribution and parallel encryption/decryption processing.
The 3rd and 4th containers are running MPI code for picture distribution and OpenMP for encrypting/decrypting parts of the picture.
When the BMP picture is ready, then it is stored in 5th container MySQL DB as BLOB - Binary Large Object.
The 5th container has 1+1 DBs - one MongoDB for collecting SNMP values from all the nodes regarding OS name, CPU and RAM usage and a MySQL DB for storing modified (encrypted/decrypted) pictures injected by OpenMPI/OpenMP clients.
All these are exposed through node.js JS Express REST Endpoints - one for SNMP values display and one for BMP picture rendering. When the zoomed BMP pic is inserted in the MySQL, then the 1st container via REST API/WebSocket is notifying the front-end with HTTP redirection to the node.js URL for downloading the picture.
* Note for MacOS AArch64 laptops you can use for MongoDb an extra container from Mongo, because the Docker Image of Ubuntu 24LTS from DockerHub critoma is only x86 64 bit for the moment.

As a list there are the following containers:
-> C01: Front-end (JS, Angular, Vue.js) -> load pic&params(operation - encryption/decryption, mode: ECB, CBC, etc) via REST API -> Java Javalin/Apache Tomcat 10 Microserv OR .NET IIS + JMS/RabbitMQ Client Publisher
-> C02: Apache TomEE 9/10 w JMS/RabbitMQ Broker - Topic & Queues
-> C03: Apache TomEE 9/10 w Java POJO/EJB Client MDB (JMS Subscriber) & OpenMPI Client <- encrypted/decrypted pics united into DB via REST API & JMS Client Publisher in Topic/Queue that the job is done
-> C03, C04: OpenMPI and OpenMP native code for encrypting/decrypting picture in parallel.
<- C01: Notification that the pic is ready => in the front-end with link for download from node.js (container C05)
-> C05: REST API in node.js w MongoDB with 2 REST API endpoints interfaces: 1 for DB access and 1 for Web Access

* SNMP and associated MongoDB is OPTIONAL and it is not subject of grading! It is inserted only for providing a self-contained solution.