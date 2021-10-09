const express = require("express");
const app = express(); 
app.listen(3000, () => console.log("listening"));   // 3000 is the arbitrary port number
app.use(express.static("public"));
app.use(express.json({limit: "1mb"}));

const bodyParser = require('body-parser');
const urlencodedParser = bodyParser.urlencoded({ extended: false});
const parseJson = require('parse-json');

const net = require('net');
const port = 8080;
//const host = 'localhost';
const host = '0.0.0.0';

var MongoClient = require('mongodb').MongoClient;
var url = "mongodb://localhost:27017/";



var received_data = '';
var temp=0;


const server = net.createServer(onClientConnection);


server.listen(port,host,function(){
   console.log(`Server started on port ${port} at ${host}`); 
});

function onClientConnection(sock){
    sock.on('data',function(data){
        received_data  = data;        
        sock.write(`${JSON.stringify(to_send)}`);
    });

    sock.on('close',function(){
        console.log(`${sock.remoteAddress}:${sock.remotePort} Terminated the connection`);
    });

    sock.on('error',function(error){
        console.error(`${sock.remoteAddress}:${sock.remotePort} Connection Error ${error}`);
    });
};

var position=1;
var sent_id=0;

var angle_0 = 0;

var status_change = 0;

function StatusCheck(){      //CHECK THAT STATUS CHANGES TO STANDBY FROM MOVING
  if(parseInt(internal[6])!=1 || status_change===2){
    status_change = 0;
  }
}
setInterval(StatusCheck,400);  //EVERY 400ms

function UpdateCommand() {  //FUNCTION THAT DECIDES WHETHER A COMMAND FROM THE WAITLIST SHOULD BE SENT
  if(waitlist[sent_id]!=undefined && waitlist[sent_id].direction == 'stop'){   //IF STOP INSTRUCTION, SEND IMMEDIATELY
    to_send = waitlist[sent_id];
    sent_id++;
    status_change=2;  //SET STATUS_CHANGE TO 2 AS THIS TIME, STATUS WILL REMAIN STANDBY
    return;
  }
  if(parseInt(internal[6])===1 && status_change===0){  //IF STATUS IS STANDBY AND WAS PREVIOUSLY MOVING
    if(waitlist[sent_id]!=undefined){ //IF THERE IS A COMMAND AT THE WAITLIST, SEND IT
      to_send = waitlist[sent_id];
      if(sent_id === ob_check-1 && ob_check!=0){   //////////////////////////////////////////
        obstacle = 0;   
        ob_check =0;
        ob_check2 = 0;                           //CHECK FOR OBSTACLE AVOIDANCE ALGORITHM//
      }                                            /////////////////////////////////////////
      sent_id++;
      status_change=1;        //SET status_change TO 1, TO STOP SENDING INSTRUCTIONS, IF STATUS DOESN'T CHANGE TO MOVING
    }
  }
}
setInterval(UpdateCommand, 500);    //EVERY 500ms


var degr;
var temp_movement;
var temp_angle; 
var function_interval = 2000;
var ob_check=0;
var ob_check2=0;
var obstacle=0;
var diff = 0;


function ObstacleCheck(){
  x_0 = internal[3];
  y_0 = internal[4];
  if(ob_check2===1){
  return;
  }
  if(ob_check===0 && ob_check2===0){
  for(var i=0; i<obstacles_y.length;i++){
    if(Math.abs(obstacles_y[i])-Math.abs(internal[4])<250){
      if(Math.abs(obstacles_x[i])-Math.abs(internal[3])>130){
      return;
      }
      else{
        diff = obstacles_x[i]-internal[3];
        if(diff>=0){
            ob_check2=1;
            obstacle = 1;
            command_id++;
            temp = to_send;
            tmp = {id : command_id , direction : 'stop', value : ''};
            waitlist.push(tmp);
            commands.push(tmp); 

            command_id++;
            degr = Math.atan(220-diff/(obstacles_y[i]-internal[4]));
            degr = degr*180/Math.PI; 
            tmp = {id : command_id , direction : 'left', value : Math.abs(degr).toString()};
            waitlist.push(tmp);
            commands.push(tmp);
            
            command_id++;
            var difference = obstacles_y[i]-internal[4]
            temp_movement = Math.sqrt((220-diff)*(220-diff)+difference*difference);
            tmp = {id : command_id , direction : 'forward', value : temp_movement.toString()};
            waitlist.push(tmp);
            commands.push(tmp);
            
            command_id++;
            temp_angle = 2*Math.abs(degr);
            tmp = {id : command_id , direction : 'right', value : temp_angle.toString()};
            waitlist.push(tmp);
            commands.push(tmp);

            command_id++;
            tmp = {id : command_id , direction : 'forward', value : temp_movement.toString()};
            waitlist.push(tmp);
            commands.push(tmp);


            command_id++;
            tmp = {id : command_id , direction : 'left', value : Math.abs(degr).toString()};
            waitlist.push(tmp);
            commands.push(tmp);

            ob_check = command_id

            break;
          }
        else if(diff<0){
          ob_check2=1;
          obstacle = 1;

          command_id++;
          temp = to_send;
          tmp = {id : command_id , direction : 'stop', value : ''};
          waitlist.push(tmp);
          commands.push(tmp); 

          command_id++;
          degr = Math.atan(220+diff/(obstacles_y[i]-internal[4]));
          degr = degr*180/Math.PI; 
          tmp = {id : command_id , direction : 'right', value : Math.abs(degr).toString()};
          waitlist.push(tmp);
          commands.push(tmp);
          
          command_id++;
          var difference = obstacles_y[i]-internal[4]
          temp_movement = Math.sqrt((220+diff)*(220+diff)+difference*difference);
          tmp = {id : command_id , direction : 'forward', value : temp_movement.toString()};
          waitlist.push(tmp);
          commands.push(tmp);
          
          command_id++;
          temp_angle = 2*Math.abs(degr);
          tmp = {id : command_id , direction : 'left', value : temp_angle.toString()};
          waitlist.push(tmp);
          commands.push(tmp);

          command_id++;
          tmp = {id : command_id , direction : 'forward', value : temp_movement.toString()};
          waitlist.push(tmp);
          commands.push(tmp);


          command_id++;
          tmp = {id : command_id , direction : 'right', value : Math.abs(degr).toString()};
          waitlist.push(tmp);
          commands.push(tmp);


          ob_check = command_id
         
          break;
        }
      }
    }
  }
}

}
setInterval(ObstacleCheck, 500);






//create obj with JSON.parse 
var obj = [];

var obstacles_x = [];
var obstacles_y = [];

var internal = [-1,0,0,0,0,0,0,0,0,0,0];


app.get("/api", (request, response) => {
  obj = csvToArray(ab2str(received_data));
  console.log(obj);
  if(parseInt(obj[0]) == 100 && obj.length == 10){  //DATA FROM CONTROL
    // internal[0] = parseInt(obj[0]);
    internal[1] = parseInt(obj[1]);       
    internal[2] = parseInt(obj[2]);
    if(Math.abs(parseInt(obj[3])-internal[3])<1000 && Math.abs(parseInt(obj[4])-internal[4])<1000){
      internal[3] = parseInt(obj[3]);
      internal[4] = parseInt(obj[4]);
    }
    internal[5] = parseInt(obj[5]);
    internal[6] = parseInt(obj[6]);
    internal[7] = parseInt(obj[7]);
    internal[8] = parseInt(obj[8]);
    internal[9] = parseInt(obj[9]);
    internal[10] = obstacle;
  }

  if(!obstacles_y.includes(internal[9]) && internal[9]!=0){    //SAVE OBSTACLES TO USE THEM FOR OBSTACLE AVOIDANCE
   obstacles_y.push(internal[9]);
  }
  if(!obstacles_x.includes(internal[8]) && internal[8]!=0){    //SAVE OBSTACLES TO USE THEM FOR OBSTACLE AVOIDANCE
    obstacles_x.push(internal[8]);
  }

  if(parseInt(obj[0]) == 123456){   ////DATA FROM ENERGY
    internal[0] = parseInt(obj[1]);
  } 
  response.json({
    battery : internal[0],  
    positionx: internal[3],
    positiony: internal[4],
    status : internal[6],
    color : internal[7],
    obstaclex: internal[8],
    obstacley: internal[9],
    avoiding: internal[10]
  });
});

var command_id = 0;
var to_send = { id: command_id, direction : '', value : ''};
commands = [];
waitlist = [];
var angle;
var angle_relative;
var degrees;
var movement;
var x_relative;
var y_relative;
var x_0;
var y_0;
var x_target;
var y_target;



app.post('/add', urlencodedParser, function(req, res){
  command_id=command_id+1;
  commands.push({ id : command_id , type : req.body.selection, direction : req.body.direction, value : req.body.value, x_coord : req.body.x, y_coord : req.body.y});
  if(commands[command_id-1].type === 'manual'){
    waitlist.push({ id: commands[command_id-1].id, direction : commands[command_id-1].direction, value: commands[command_id-1].value});
    // if(waitlist[waitlist.length - 1].direction == 'left'){
    //   angle_0 = angle_0 - waitlist[waitlist.length - 1].value
    // }
    // if(waitlist[waitlist.length - 1].direction == 'right'){
    //   angle_0 = angle_0 + waitlist[waitlist.length - 1].value
    // }
  }
  
  else if(commands[command_id-1].type === 'auto'){
      x_target = parseInt(commands[command_id-1].x_coord);
      y_target = parseInt(commands[command_id-1].y_coord);
      x_0 = internal[3]; //current X coordinate from drive
      y_0 = internal[4]; //current Y coordinate from drive
      angle_0 = internal[5]; //current angle from drive
      x_relative = x_target - x_0;
      y_relative = y_target - y_0;
      
      degrees = Math.atan(x_relative/y_relative);   //CALCULATE THE INVERSE TANGENT TO FIND THE ANGLE
      degrees = degrees*180/Math.PI;                //CHANGE TO DEGREES
      movement = Math.sqrt(x_relative*x_relative + y_relative*y_relative);   //CALCULATE THE HYPOTENUSE
      
      if(x_relative>=0 && y_relative>=0){          
        angle = degrees;
      }
      else if(x_relative<=0 && y_relative<=0){
        angle = -180+degrees;
      }
      else if(x_relative>=0 && y_relative<=0){
        angle = 180 - Math.abs(degrees);
      }
      else if(x_relative<=0 && y_relative>=0){
        angle = degrees;
      }                                       //DECIDE THE ANGLE TO BE SUBMITTED ON THE COMMAND                                                
      angle_relative = angle - angle_0        //BASED ON THE CURRENT ANGLE OF THE ROVER    



      if(angle_relative>=0){
        tmp = {id : command_id , direction : 'right', value : angle_relative.toString()};
      }
      else if(angle_relative<0){

        tmp = {id : command_id , direction : 'left', value : (-angle_relative).toString()};
      }
      waitlist.push(tmp);
      //angle_0 = angle;
      command_id++;
      commands.push({ id : command_id , type : req.body.selection, direction : req.body.direction, value : req.body.value, x_coord : req.body.x, y_coord : req.body.y});
      tmp = {id : command_id , direction : 'forward', value : movement.toString()};
      waitlist.push(tmp);
    }

    else if(commands[command_id-1].type === 'reset'){
      waitlist.push({ id: commands[command_id-1].id, type: 'reset', direction : '', value: ''});
    }
  
    else if(commands[command_id-1].type === 'stop'){
      waitlist.push({ id: commands[command_id-1].id, type: 'stop', direction : '', value: ''});
    }

  console.log(JSON.stringify(commands[commands.length - 1]));
  res.status(204).send();
  // res.sock.write(`${JSON.stringify(to_send)}`);
  // sock.write(`You Said ${response}`);
})


function ab2str(buf) {
  return String.fromCharCode.apply(null, new Uint16Array(buf));
}


function csvToArray(str, delimiter = ",") {

  const headers = str.slice().split(delimiter);

  return headers;
}