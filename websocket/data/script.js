var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

// Init web socket when the page loads
window.addEventListener("load", onLoad);

function onLoad(event) {
  initWebSocket();
  initButton();
}

// Initialize WebSocket
function initWebSocket() {
  console.log("Trying to open a WebSocket connection…");
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

// When WebSocket is established, call the getReadings() function
function onOpen(event) {
  console.log("Connection opened");
  getReadings();
}

// Reconnect WebSocket when closed
function onClose(event) {
  console.log("Connection closed");
  setTimeout(initWebSocket, 2000);
}

// Send "getReadings" to ESP32 to fetch sensor data
function getReadings() {
  websocket.send("getReadings");
}

// Process messages received from ESP32
function onMessage(event) {
  console.log(event.data);
  try {
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++) {
      var key = keys[i];
      document.getElementById(key).innerHTML = myObj[key];
    }
  } catch (e) {
    console.error("Invalid JSON received:", event.data);
  }
}

// Initialize button functionality
function initButton() {
  const button = document.getElementById("sendSignalButton");
  if (button) {
    button.addEventListener("click", () => {
      console.log("Button clicked. Sending signal to ESP32.");
      websocket.send("buttonClicked");
    });
  } else {
    console.error("Button element not found!");
  }
}
