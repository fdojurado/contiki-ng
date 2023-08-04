/* A simple log file generator script */

TIMEOUT(1800000); /* 3600 seconds or 1 hour */

// Create Latency class
function Latency(emission_time, reception_time) {
  this.emission_time = emission_time;
  this.reception_time = reception_time;
  this.latency = reception_time - emission_time;
}

// Create a Node class
function Node(id) {
  this.id = id;
  // Key is the SEQ number and value is the Latency object
  this.latency = {};
}
Node.prototype.addLatency = function (seq, latency) {
  this.latency[seq] = latency;
};
Node.prototype.printLatencySamples = function () {
  log.log("Node " + this.id + " latency samples:\n");
  // Print the dictionary
  for (var key in this.latency) {
    if (this.latency.hasOwnProperty(key)) {
      log.log(
        "SEQ: " +
          key +
          " emission time: " +
          this.latency[key].emission_time +
          " reception time: " +
          this.latency[key].reception_time +
          " latency: " +
          this.latency[key].latency +
          "\n"
      );
    }
  }
};
// Create a network class to hold network information
function Network() {
  this.nodes = {};
}
Network.prototype.addNode = function (id) {
  this.nodes[id] = new Node(id);
  return this.nodes[id];
};
Network.prototype.getNode = function (id) {
  return this.nodes[id];
};
Network.prototype.addLatencySample = function (
  id,
  seq,
  emission_time,
  reception_time
) {
  if (this.nodes[id] == undefined) {
    this.addNode(id);
  }
  this.nodes[id].addLatency(seq, new Latency(emission_time, reception_time));
};
Network.prototype.updateReceptionTime = function (id, seq, reception_time) {
  // Get the node
  network_node = this.getNode(id);
  // Return if the node is not in the network
  if (network_node == undefined) {
    return;
  }
  // If the SEQ is not in the latency dictionary, return
  if (network_node.latency[seq] == undefined) {
    return;
  }
  // Update the reception time
  network_node.latency[seq].reception_time = reception_time;
  // Update the latency, this time is in microseconds
  network_node.latency[seq].latency =
    network_node.latency[seq].reception_time -
    network_node.latency[seq].emission_time;
};
Network.prototype.printLatencySamples = function () {
  var keys = Object.keys(this.nodes);
  keys.forEach(
    function (key) {
      network_node = this.getNode(key);
      // network_node.printLatencySamples();
      latency_samples = network_node.latency;
      number_of_samples = Object.keys(latency_samples).length;
      log.log("Node " + key + " latency samples: [");
      for (var key in latency_samples) {
        if (latency_samples.hasOwnProperty(key)) {
          if (latency_samples[key].latency > 0) {
            if (number_of_samples == 1) {
              log.log(latency_samples[key].latency + "]\n");
              break;
            }
            log.log(latency_samples[key].latency + ", ");
          }
          if (number_of_samples == 1) {
            log.log("]\n");
            break;
          }
          number_of_samples--;
        }
      }
    }.bind(this)
  );
};
// Create a network object
var network = new Network();

log.log("Starting COOJA logger\n");

timeout_function = function () {
  log.log("Script timed out.\n");
  network.printLatencySamples();
  log.testOK();
};

start_samples = false;

while (true) {
  YIELD();
  //   We only start collecting latency samples after motes print "[INFO: SA"
  //   This is to avoid collecting latency samples from the booting process
  if (msg.contains("[INFO: SA")) {
    start_samples = true;
  }
  if (start_samples) {
    if (msg.contains("Sending Data pkt (SEQ:")) {
      // Get the SEQ number which is after "SEQ:"
      seq = msg.split("SEQ:")[1];
      // The number goes up to the ) character
      seq = seq.split(")")[0];
      // Convert the SEQ number to integer
      seq = Number(seq);
      // This time is in microseconds
      network.addLatencySample(id, seq, time, 0);
    }
    if (msg.contains("Received Data pkt from:")) {
      // Get the source node ID which is after "Received Data pkt from:"
      source_id = msg.split("Received Data pkt from:")[1];
      // split by comma
      source_id = source_id.split(",")[0];
      // Split by . and get the last element
      source_id = source_id.split(".")[1];
      // Convert the string to a number
      source_id = Number(source_id);
      // Get the SEQ number which is after "SEQ:"
      seq = msg.split("SEQ:")[1];
      // Convert the SEQ number to integer
      seq = Number(seq);
      // Update the reception time
      network.updateReceptionTime(source_id, seq, time);
    }
  }
}
