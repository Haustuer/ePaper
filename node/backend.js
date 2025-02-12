const WebSocket = require('ws');
const socket = new WebSocket("wss://stream.aisstream.io/v0/stream")
const WebSocket_port = 8081
const http = require("http");

// REST API
const RestAPI_port = 10443;

const express = require('express');
const app = express();
app.get('/', (req, res) => {
    res.header("Access-Control-Allow-Origin", "*");
    res.header("Access-Control-Allow-Headers", "X-Requested-With");
    res.send('REST API ');
  });
  
  // Define routes
  app.get('/ships', (req, res) => {
    res.header("Access-Control-Allow-Origin", "*");
    res.header("Access-Control-Allow-Headers", "X-Requested-With");
    res.send(JSON.stringify(trackedObjects));
  });
    // Define routes
    app.get('/sprites', (req, res) => {

      let wid=1872;
      let hig=1404;
    

      objectsToDraw=[];
      trackedObjects.ships.forEach((thing)=>{
       const result = mercatorProjection(thing.position[0], thing.position[1]);
       // const result = scaleToMap(result1[0], result1[1]);
        let newThing={"x":Math.round((wid*result[0])/100),"y":Math.round((hig*result[1])/100),"icon":1};

        objectsToDraw.push(newThing);
      })
      trackedObjects.planes.forEach((thing)=>{
        const result = mercatorProjection(thing.position[0], thing.position[1]);
       //const result = scaleToMap(result1[0], result1[1]);
       let newThing={"x":Math.round((wid*result[0])/100),"y":Math.round((hig*result[1])/100),"icon":2};

       objectsToDraw.push(newThing);
      })
      trackedObjects.otherObjects.forEach((thing)=>{
        const result = mercatorProjection(thing.position[0], thing.position[1]);
      //  const result = scaleToMap(result1[0], result1[1]);
      let newThing={"x":Math.round((wid*result[0])/100),"y":Math.round((hig*result[1])/100),"icon":3};

      objectsToDraw.push(newThing);
      })

      res.header("Access-Control-Allow-Origin", "*");
      res.header("Access-Control-Allow-Headers", "X-Requested-With");
      res.send(JSON.stringify(objectsToDraw));
    });
  app.get('/shipsList', (req, res) => {
    res.header("Access-Control-Allow-Origin", "*");
    res.header("Access-Control-Allow-Headers", "X-Requested-With");
    res.end(JSON.stringify(ShipList));
  });
  
  app.listen(RestAPI_port, () => {
    console.log(`Server running at http://localhost:${RestAPI_port}`);
  });
  

  // DATA Structur
  var trackedObjects = {
    "ships": [],
    "planes": [],
    "otherObjects": [ {
      "name":"Titanic",
      "position":[41.7325, -49.9469],
      "source": "Fix",
      "type" : "Wreck",
      "MMSI": 912320328
    }
    ],
  }  

  var objectsToDraw=[{"x":50,"y":25,"icon":2},{"x":20,"y":70,"icon":1},{"x":20,"y":90,"icon":2},{"x":500,"y":250,"icon":2},{"x":1020,"y":900,"icon":1},{"x":666,"y":666,"icon":2}];


  var ShipList = ["211238300", "211735050", "211713930", "353136000", "368207620", "367719770", "211476060", "228131430", "211222710"];

  // Constants
const R = 6371.0; // Radius of the Earth in kilometers

// Convert degrees to radians
function toRadians(degrees) {
    return degrees * Math.PI / 180.0;
}

// Mercator projection function
function mercatorProjection(latitude, longitude) {
    const phi = toRadians(latitude);   // Convert latitude to radians
    const lambda = toRadians(longitude); // Convert longitude to radians

    // Mercator projection
    const x = R * lambda;
    const y = R * Math.log(Math.tan(Math.PI / 4.0 + phi / 2.0));

    // Convert to percentage
    const xPercent = ((lambda + Math.PI) / (2 * Math.PI)) * 100.0;
    const yPercent = ((phi + (Math.PI / 2.0)) / Math.PI) * 100.0;

    return [ xPercent, yPercent ];
}
  /* ---------------------------------------------
     Connecting and handling AIS Stream API
   --------------------------------------------- */
  let sockets = [];
  socket.onopen = function (_) {
    let subscriptionMessage = {
      Apikey: "f250a5ee54279fb5f5ace67cafa8adec51a4a169",
      BoundingBoxes: [[[-90, -180], [90, 180]]],
      FiltersShipMMSI: ShipList, // Optional!
      FilterMessageTypes: ["PositionReport"] // Optional!
    }
    socket.send(JSON.stringify(subscriptionMessage));
  };
  
  socket.onmessage = function (event) {
    let aisMessage = JSON.parse(event.data)
    //console.log(aisMessage)
    let obj={}
    obj.name = aisMessage.MetaData.ShipName
    obj.MMSI = aisMessage.MetaData.MMSI
    obj.position = [aisMessage.Message.PositionReport.Latitude, aisMessage.Message.PositionReport.Longitude];
    obj.timeStamp = aisMessage.MetaData.time_utc
    obj.source = "AisStream"
    addObjToTracker(obj, trackedObjects.ships) 
  };
  /* -------------------------------------------------------
     Connecting and handling Local Web Socket for server
   ------------------------------------------------------ */
  
  const server = new WebSocket.Server({
    port: WebSocket_port
  });
  
  server.on('connection', function (socket) {
    sockets.push(socket);
    //console.log(sockets);
    // When you receive a message, send that message to every socket.
    socket.on('message', function (msg) {
      //sockets.forEach(s => s.send(msg));   
      //console.log(msg)    ;
      let updateMSG = {};
      trackedObjects.ships.forEach(ship => {
        updateMSG.type = "Ship";
        updateMSG.state = "New";
        updateMSG.content = ship;
        socket.send(JSON.stringify(updateMSG))      
      }
      );
      trackedObjects.otherObjects.forEach(ship => {
        updateMSG.type = "Object";
        updateMSG.state = "New";
        updateMSG.content = ship;
        socket.send(JSON.stringify(updateMSG))      
      });
      trackedObjects.planes.forEach(ship => {
        updateMSG.type = "Plane";
        updateMSG.state = "New";
        updateMSG.content = ship;
        socket.send(JSON.stringify(updateMSG))    
      });
    });
    // When a socket closes, or disconnects, remove it from the array.
    socket.on('close', function () {
      sockets = sockets.filter(s => s !== socket);
    });
  });
  
  /* ---------------------------------------------------------
      This part fetches the ISS Location from wheretheiss.at
  ----------------------------------------------------------*/
  setInterval(() => {
    fetch('https://api.wheretheiss.at/v1/satellites/25544')
      .then(response => {
        // console.log(response)
        if (response.status == 200) {       
          return response.json()
        }      
      })
      .then(data => {
        // console.log(data);
        return [data.latitude, data.longitude]
      }).then(ShipPos => {
        let objID = 25544
        let obj = {}
        obj.MMSI = objID
        obj.name = "ISS"
        obj.source = "wheretheiss.at"
        obj.position = ShipPos
        obj.type = "Satelit"
        obj.timeStamp = new Date()         
        addObjToTracker(obj, trackedObjects.otherObjects)
      })
  }, 5000)
  /* -------------------------------------------------------------------
  this function adds new objefts to the tracker 
  and updates existing ones
  finally all updates will be published to all websocket rtecivers
---------------------------------------------------------------------*/
function addObjToTracker(obj, database) {
    //console.log(obj)
    let myobj = database.find(ship => ship.MMSI === obj.MMSI);
    let updateMSG = {};
    let thisType;
    switch (database) {
      case trackedObjects.ships:
        thisType = "Ship"
        break;
      case trackedObjects.otherObjects:
        thisType = "Object";
        break;
      case trackedObjects.planes:
        thisType = "Plane"
        break;
      default:
        thisType = "unknown"
        break
    }
    updateMSG.type=thisType;
    updateMSG.content = obj;
    if (typeof myobj !== 'undefined' && myobj !== null) {
      myobj.position = obj.position
      myobj.timeStamp=obj.timeStamp   
      updateMSG.state = "Update";
  
    } else {     
      database.push(obj);    
      updateMSG.state = "New";  
  
    }
    console.log(obj.name+": "+updateMSG.state)   
    sockets.forEach(s => s.send(JSON.stringify(updateMSG)));
  }