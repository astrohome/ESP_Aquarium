var rainbowEnable = false;
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
connection.onopen = function () {
    connection.send('Connect ' + new Date());
};
connection.onerror = function (error) {
    console.log('WebSocket Error ', error);
};
connection.onmessage = function (e) {  
    console.log('Server: ', e.data);
    var data = JSON.parse(e.data);
    if (!data.time) {
        for (var i = 0; i < 4; i++) {
            document.getElementById(String.fromCharCode('a'.charCodeAt(0) + i)).value = data[i];
        }
    } else {
        document.getElementById("time").innerText = data.time.hour + ":" + data.time.min;
    }
};
connection.onclose = function(){
    console.log('WebSocket connection closed');
};

function sendRGB(id, value) {
    connection.send(id + value);
}

function moonlight() {
    connection.send('M');
}

function off() {
    connection.send('0');
}

function on() {
    connection.send('1');
}

function save() {
    connection.send('S');
}

function load() {
    connection.send('L');
}
