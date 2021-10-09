const net = require('net');
//define the server port and host
const port = 8080;
const host = 'localhost';
//Create an instance of the socket client.
const client = new net.Socket();
//Connect to the server using the above defined config.

client.connect(port,host,function(){
   console.log(`Connected to server on ${host}:${port}`);
   //Connection was established, now send a message to the server.
//    client.write('100,45,40,2,-20,15'); 
   client.write('123,58');
   // client.write('100,100,100,2,-20,15')
   // client.write('{"battery":"86","position":"5","status":"1"}');
   // client.write('4');
});

//Add a data event listener to handle data coming from the server
client.on('data',function(data){
   console.log(`Server Says : ${data}`); 
});

//Add Client Close function
client.on('close',function(){
   console.log('Connection Closed');
});
//Add Error Event Listener
client.on('error',function(error){
   console.error(`Connection Error ${error}`); 
});