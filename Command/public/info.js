
var final_position;
var result = 2;
var x_pos = [0];
var y_pos = [0];

var x_obs_g = [];
var y_obs_g = [];

var x_obs_p = [];
var y_obs_p = [];

function refresh() {

const getvel = fetchdata().then(data => {
        return [data.battery, data.positionx, data.positiony, data.status, data.color, data.obstaclex, data.obstacley, data.avoiding];
    });
    
    // update variable values according to data received from server
    const get = async() => {
        tmp = await getvel;
        console.log(tmp)
        updateBatteryLevel(tmp[0]);       // player
        updatePosition(tmp[1],tmp[2]);
        updateStatus(tmp[3]);
        updateObstacles(tmp[4],tmp[5],tmp[6]);
        avoidingObstacle(tmp[7]);
    }
    get();
}
setInterval(refresh, 500);
    


async function fetchdata() {
    const option = {
        method: "GET",
        headers: {
            "Content-Type": "application/json"
        }
    };
    const response = await fetch("/api", option);
    return response.json();
}


function updateBatteryLevel(tmp){
    // console.log(tmp);
    if(tmp>=0 && tmp<=100){
        document.getElementById("batt").innerHTML = tmp;
    }
    else{
    document.getElementById("batt").innerHTML = "Unknown";
    }
}

function updateStatus(tmp){
console.log(tmp);
// result = String.fromCharCode.apply(null, tmp);
if(tmp==0){
    document.getElementById("status").innerHTML = "Off";
    document.getElementById("change").className = "fa fa-lock w3-xxxlarge";
}
else if(tmp==1){
    document.getElementById("status").innerHTML = "Standby";
    document.getElementById("change").className = "fa fa-unlock w3-xxxlarge";
}
else if(tmp==2){
document.getElementById("status").innerHTML = "Moving Forward";
document.getElementById("change").className = "fa fa-arrow-up w3-xxxlarge";
}
else if(tmp==3){
    document.getElementById("status").innerHTML = "Moving Backwards";
    document.getElementById("change").className = "fa fa-arrow-down w3-xxxlarge";
}
else if(tmp==4){
    document.getElementById("status").innerHTML = "Turning Right";
    document.getElementById("change").className = "fa fa-arrow-right w3-xxxlarge";
}
else if(tmp==5){
    document.getElementById("status").innerHTML = "Turning Left";
    document.getElementById("change").className = "fa fa-arrow-left w3-xxxlarge";
}
else{
    document.getElementById("status").innerHTML = "Unknown";
}
}


function updatePosition(x,y){

    // console.log(final_position);
    if (Number.isInteger(x)) {
        x_pos.push(x/10);
    }
    if (Number.isInteger(y)) {
        y_pos.push(y/10);
    }
    // x_pos.push(x);
    // y_pos.push(y);
    document.getElementById("posx").innerHTML = x/10;
    document.getElementById("posy").innerHTML = y/10;
}

function updateObstacles(c,x,y){

    // console.log(final_position);
    // x=x/10;
    // y=y/10;
    if(c!=0){
        if (Number.isInteger(x)) {
            if(c===1){
                if(!x_obs_g.includes(x/10)){
                    x_obs_g.push(x/10);
                    document.getElementById("obsx").innerHTML = x/10;
                }
            }
            else if(c===2){
                if(!x_obs_p.includes(x/10)){
                    x_obs_p.push(x/10);
                    document.getElementById("obsx").innerHTML = x/10;
                }
            }
        }
        if (Number.isInteger(y)) {
            if(c===1){
                if(!y_obs_g.includes(y/10)){
                    y_obs_g.push(y/10);
                    document.getElementById("obsy").innerHTML = y/10;
                }
            }
            else if(c===2){
                if(!y_obs_p.includes(y/10)){
                    y_obs_p.push(y/10);
                    document.getElementById("obsy").innerHTML = y/10;
                }
            }
        }
    }
}

function avoidingObstacle(tmp){
    // result = String.fromCharCode.apply(null, tmp);
    if(tmp==0){
        document.getElementById("obstacle").innerHTML = "";
    }
    else if(tmp==1){
        document.getElementById("obstacle").innerHTML = "!!Avoiding Obstacle!!";
    }
    else{
    document.getElementById("obstacle").innerHTML = "";
    }   
}

function ab2str(buf) {
    return String.fromCharCode.apply(null, new Uint16Array(buf));
}

