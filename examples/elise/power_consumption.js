/* A simple log file generator script */

TIMEOUT(900000); /* 3600 seconds or 1 hour */
// Create a Node class
function Node(id) {
  this.id = id;
  this.power = [];
}
Node.prototype.addPower = function (power) {
  this.power.push(power);
};
Node.prototype.printPowerSamples = function () {
  log.log("Node " + this.id + " power samples:\n");
  for (i = 0; i < this.power.length; i++) {
    log.log(this.power[i] + "\n");
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
Network.prototype.addPowerSample = function (id, power) {
  if (this.nodes[id] == undefined) {
    this.addNode(id);
  }
  this.nodes[id].addPower(power);
};
Network.prototype.printPowerSamples = function () {
  var keys = Object.keys(this.nodes);
  keys.forEach(
    function (key) {
      network_node = this.getNode(key);
      power_samples = network_node.power;
      log.log("Node " + key + " power samples: [");
      for (i = 0; i < power_samples.length; i++) {
        if (i == power_samples.length - 1) {
          log.log(power_samples[i] + "]\n");
          break;
        }
        log.log(power_samples[i] + ", ");
      }
    }.bind(this)
  );
};
// Create a network object
var network = new Network();

log.log("Starting COOJA logger\n");

timeout_function = function () {
  log.log("Script timed out.\n");
  network.printPowerSamples();
  log.testOK();
};

start_samples = false;

while (true) {
  YIELD();
  //   We only start collecting power samples after motes print "[INFO: SA"
  //   This is to avoid collecting power samples from the booting process
  if (msg.contains("[INFO: SA")) {
    start_samples = true;
  }
  if (start_samples) {
    //   We only collect power samples from motes which have the SDN-POWER module
    if (msg.contains("[INFO: SDN-POWER ] Power (uW)")) {
      // Split the message by '/'
      split_msg = msg.split("/");
      // Get the number which is before the first '/'
      split_msg = split_msg[0];
      // Split the message by ':'
      split_msg = split_msg.split(":");
      // Get the number which is after the first ':'
      split_msg = split_msg[2];
      // Convert the string to a number
      power = Number(split_msg);
      // Get the node id which is after the first '/'
      // log.log("Node " + id + " power: " + power + "\n");
      network.addPowerSample(id, power);
    }
  }
}
