/* A simple log file generator script */

TIMEOUT(900000); /* 3600 seconds or 1 hour */

// Create a Node class
function Node(id) {
  this.id = id;
  // Packet counters
  this.num_packets = 0;
  this.packet_size = 0;
  // Throughput samples
  this.throughput = [];
}
Node.prototype.addPacket = function (packet_size) {
  this.num_packets++;
  this.packet_size += parseInt(packet_size);
};
Node.prototype.printPacketCounters = function () {
  log.log(
    "Node " +
      this.id +
      " packet counters: num_packets: " +
      this.num_packets +
      " packet_size: " +
      this.packet_size +
      "\n"
  );
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
Network.prototype.addPacket = function (id, packet_size) {
  if (this.nodes[id] == undefined) {
    this.addNode(id);
  }
  this.nodes[id].addPacket(packet_size);
};
Network.prototype.clearPacketCounters = function () {
  var keys = Object.keys(this.nodes);
  keys.forEach(
    function (key) {
      network_node = this.getNode(key);
      network_node.num_packets = 0;
      network_node.packet_size = 0;
    }.bind(this)
  );
};
Network.prototype.calculateThroughput = function () {
  var keys = Object.keys(this.nodes);
  keys.forEach(
    function (key) {
      network_node = this.getNode(key);
      // Calculate the throughput
      throughput = (network_node.packet_size * 8) / 60;
      network_node.throughput.push(throughput);
    }.bind(this)
  );
};
Network.prototype.printThroughputSamples = function () {
  var keys = Object.keys(this.nodes);
  keys.forEach(
    function (key) {
      network_node = this.getNode(key);
      throughput_samples = network_node.throughput;
      log.log("Node " + key + " throughput samples: [");
      // Return if there are no throughput samples
      if (throughput_samples.length == 0) {
        log.log("]\n");
        return;
      }
      for (i = 0; i < throughput_samples.length; i++) {
        if (i == throughput_samples.length - 1) {
          log.log(throughput_samples[i] + "]\n");
          break;
        }
        log.log(throughput_samples[i] + ", ");
      }
    }.bind(this)
  );
};
Network.prototype.printPacketCounters = function () {
  var keys = Object.keys(this.nodes);
  keys.forEach(
    function (key) {
      network_node = this.getNode(key);
      network_node.printPacketCounters();
    }.bind(this)
  );
};

// Create a network object
var network = new Network();

log.log("Starting COOJA logger\n");

timeout_function = function () {
  log.log("Script timed out.\n");
  network.printThroughputSamples();
  log.testOK();
};

start_samples = false;
start_samples_time = 0;
throughput_time = 60000000; /* 60 seconds */

while (true) {
  YIELD();
  //   We only start collecting throughput samples after motes print "[INFO: SA"
  //   This is to avoid collecting throughput samples from the booting process
  if (msg.contains("[INFO: SA")) {
    start_samples = true;
    start_samples_time = time;
  }
  if (start_samples) {
    if (
      msg.contains("[INFO: TSCH-LOG") &&
      msg.contains("link  2  17") &&
      msg.contains("tx") &&
      msg.contains("st 0")
    ) {
      // Get the packet size which is the number after the "len " keyword
      packet_size = msg.split("len ")[1];
      // Split by ',' and pick the first element
      packet_size = packet_size.split(",")[0];
      packet_size = Number(packet_size);
      network.addPacket(id, packet_size);
    }
    //  We only collect throughput samples for 60 seconds
    if (time - start_samples_time > throughput_time) {
      // network.printPacketCounters();
      // Calculate the throughput
      network.calculateThroughput();
      // Restart the throughput timer
      start_samples_time = time;
      // Clear packet counters
      network.clearPacketCounters();
    }
  }
}
