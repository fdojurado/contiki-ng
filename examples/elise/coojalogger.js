/* A simple log file generator script */

// TIMEOUT(144000000); /* 3600 seconds or 1 hour */
TIMEOUT(1800000); /* 3600 seconds or 1 hour */

packet_size = 11;
sample_time = 60; // 1 minute

time_aux = 0;
time_difference = 0;
// Throughput calculation based on the number of messages received
function calculateThroughput(num_pkts) {
  // Calculate the throughput
  throughput = num_pkts * packet_size * 8 / sample_time;
  return throughput;
}
// Create constructor function for statistics
function Statistics() {
  this.sent_data_pkts = 0;
  this.sent_na_pkts = 0;
  this.sink_rx_data_pkts = 0;
  this.sink_rx_na_pkts = 0;
  this.forwarded_pkts = 0;
  this.mac_tx_data = 0;
  this.mac_rx_data = 0;
  this.mac_tx_na = 0;
  this.mac_rx_na = 0;
}

Statistics.prototype.print = function () {
  log.log(
    "Sent Data Pkts: " +
    this.sent_data_pkts +
    " | Sent NA Pkts: " +
    this.sent_na_pkts +
    " | Sink Rx Data Pkts: " +
    this.sink_rx_data_pkts +
    " | Sink Rx NA Pkts: " +
    this.sink_rx_na_pkts +
    " | Forwarded Pkts: " +
    this.forwarded_pkts +
    " | MAC Tx Data: " +
    this.mac_tx_data +
    " | MAC Rx Data: " +
    this.mac_rx_data +
    " | MAC Tx NA: " +
    this.mac_tx_na +
    " | MAC Rx NA: " +
    this.mac_rx_na +
    "\n"
  );
};
// Create a Node class
function Node(id) {
  this.id = id;
  this.statistics = new Statistics();
}
Node.prototype.addForwardedPacket = function () {
  this.statistics.forwarded_pkts++;
}
Node.prototype.addSentDataPkt = function () {
  this.statistics.sent_data_pkts++;
}
Node.prototype.addSentNAPkt = function () {
  this.statistics.sent_na_pkts++;
}
Node.prototype.addSinkRxDataPkt = function () {
  this.statistics.sink_rx_data_pkts++;
}
Node.prototype.addSinkRxNAPkt = function () {
  this.statistics.sink_rx_na_pkts++;
}
Node.prototype.addMacTxData = function () {
  this.statistics.mac_tx_data++;
}
Node.prototype.addMacRxData = function () {
  this.statistics.mac_rx_data++;
}
Node.prototype.addMacTxNA = function () {
  this.statistics.mac_tx_na++;
}
Node.prototype.addMacRxNA = function () {
  this.statistics.mac_rx_na++;
}
Node.prototype.getSentDataPkt = function () {
  return this.statistics.sent_data_pkts;
}
Node.prototype.getSentNAPkt = function () {
  return this.statistics.sent_na_pkts;
}
Node.prototype.getSinkRxDataPkt = function () {
  return this.statistics.sink_rx_data_pkts;
}
Node.prototype.getSinkRxNAPkt = function () {
  return this.statistics.sink_rx_na_pkts;
}
Node.prototype.getSentPkts = function () {
  return this.statistics.sent_data_pkts + this.statistics.sent_na_pkts;
}
Node.prototype.getSinkRxPkts = function () {
  return this.statistics.sink_rx_data_pkts + this.statistics.sink_rx_na_pkts;
}
Node.prototype.getMacTxData = function () {
  return this.statistics.mac_tx_data;
}
Node.prototype.getMacRxData = function () {
  return this.statistics.mac_rx_data;
}
Node.prototype.getMacTxNA = function () {
  return this.statistics.mac_tx_na;
}
Node.prototype.getMacRxNA = function () {
  return this.statistics.mac_rx_na;
}
Node.prototype.getMacTx = function () {
  return this.statistics.mac_tx_data + this.statistics.mac_tx_na;
}
Node.prototype.printStatistics = function () {
  log.log("Node " + this.id + " statistics:\n");
  this.statistics.print();
}
Node.prototype.clearStatistics = function () {
  this.statistics.sent_data_pkts = 0;
  this.statistics.sent_na_pkts = 0;
  this.statistics.sink_rx_data_pkts = 0;
  this.statistics.sink_rx_na_pkts = 0;
  this.statistics.forwarded_pkts = 0;
  this.statistics.mac_tx_data = 0;
  this.statistics.mac_rx_data = 0;
  this.statistics.mac_tx_na = 0;
  this.statistics.mac_rx_na = 0;
}
// Calculate the throughput for the node
Node.prototype.calculateThroughputAtSink = function () {
  return calculateThroughput(this.getSinkRxPkts());
}
Node.prototype.calculateThroughputAtNode = function () {
  return calculateThroughput(this.getMacTx());
}

// Create a network class to hold network information
function Network() {
  this.nodes = {}
  this.statistics = new Statistics();
}
Network.prototype.addNode = function (id) {
  this.nodes[id] = new Node(id);
  return this.nodes[id];
}
Network.prototype.getNode = function (id) {
  // log.log("Getting node " + id + "\n")
  return this.nodes[id];
}
Network.prototype.addSinkRxDataPkt = function (id) {
  this.statistics.sink_rx_data_pkts++;
  // Check if the node exists
  if (this.getNode(id) == undefined) {
    this.addNode(id);
  }
  this.getNode(id).addSinkRxDataPkt();
}
Network.prototype.addSinkRxNAPkt = function (id) {
  this.statistics.sink_rx_na_pkts++;
  // Check if the node exists
  if (this.getNode(id) == undefined) {
    this.addNode(id);
  }
  this.getNode(id).addSinkRxNAPkt();
}
Network.prototype.getSinkRxDataPkt = function () {
  return this.statistics.sink_rx_data_pkts;
}
Network.prototype.getSinkRxNAPkt = function () {
  return this.statistics.sink_rx_na_pkts;
}
Network.prototype.printStatistics = function () {
  log.log("Network statistics:\n");
  this.statistics.print();
}
Network.prototype.printNodesStatistics = function () {
  var keys = Object.keys(this.nodes);
  keys.forEach(function (key) {
    log.log("key: " + key + "\n")
    network_node = this.getNode(key);
    network_node.printStatistics();
  }
    .bind(this));
}
Network.prototype.clearStatistics = function () {
  this.statistics.sent_data_pkts = 0;
  this.statistics.sent_na_pkts = 0;
  this.statistics.sink_rx_data_pkts = 0;
  this.statistics.sink_rx_na_pkts = 0;
  this.statistics.forwarded_pkts = 0;
  this.statistics.mac_tx_data = 0;
  this.statistics.mac_rx_data = 0;
  this.statistics.mac_tx_na = 0;
  this.statistics.mac_rx_na = 0;
}
Network.prototype.clearNodesStatistics = function () {
  var keys = Object.keys(this.nodes);
  keys.forEach(function (key) {
    network_node = this.getNode(key);
    network_node.clearStatistics();
  }
    .bind(this));
}
// Calculate the network throughput using
// the sink rx packets and the sample_time
Network.prototype.calculateNetworkThroughput = function () {
  return calculateThroughput(this.getSinkRxDataPkt() + this.getSinkRxNAPkt());
}
// Calculate the throughput for each node in the network and put in a dictionary
Network.prototype.calculateThroughputAtSink = function () {
  var nodes_throughput = {};
  var keys = Object.keys(this.nodes);
  keys.forEach(function (key) {
    network_node = this.getNode(key);
    nodes_throughput[key] = network_node.calculateThroughputAtSink();
  }
    .bind(this)
  );
  return nodes_throughput;
}
// Calculate the average throughput for all nodes in the network
Network.prototype.calculateThroughputAtNode = function () {
  var nodes_throughput = {};
  var keys = Object.keys(this.nodes);
  keys.forEach(function (key) {
    network_node = this.getNode(key);
    nodes_throughput[key] = network_node.calculateThroughputAtNode();
  }
    .bind(this)
  );
  return nodes_throughput;
}

// Create a network object
var network = new Network();

log.log("Starting COOJA logger\n");

timeout_function = function () {
  log.testOK();
};



function average(list) {
  sum = 0;
  for (i = 0; i < list.length; i++) {
    sum += list[i];
  }
  average_list = sum / list.length;
  return average_list;
}

while (true) {
  YIELD();
  if (msg.contains("Data message received from ")) {
    // log.log("Data message received\n");
    // Get the node id, which is at the end of the message
    node_id = msg.slice(-2);
    // log.log("Node id: " + node_id + "\n");
    // Split the string by the . character if it exists
    if (node_id.contains(".")) {
      node_id = node_id.split(".")[1];
    }
    // log.log("Node id splitted: " + node_id + "\n");
    // Network number of messages received
    // log.log("Adding Data packet to network statistics\n");
    network.addSinkRxDataPkt(node_id);
  }
  if (msg.contains("NA message received from ")) {
    // log.log("NA message received\n");
    // Get the node id, which is at the end of the message
    node_id = msg.slice(-2);
    // log.log("Node id: " + node_id + "\n");
    // Split the string by the . character if it exists
    if (node_id.contains(".")) {
      node_id = node_id.split(".")[1];
    }
    // log.log("Node id splitted: " + node_id + "\n");
    // Add packet to network statistics
    // log.log("Adding NA packet to network statistics\n");
    network.addSinkRxNAPkt(node_id);
  }
  if (msg.contains("Sending Data pkt")) {
    // Check if the node exists
    src_node = network.getNode(id);
    if (src_node == undefined) {
      src_node = network.addNode(id);
    }
    // log.log("Node: " + node.id + "\n");
    src_node.addSentDataPkt();

  }
  if (msg.contains("Sending NA pkt")) {
    // Check if the node exists
    src_node = network.getNode(id);
    if (src_node == undefined) {
      src_node = network.addNode(id);
    }
    // log.log("Node: " + node.id + "\n");
    src_node.addSentNAPkt();

  }
  if (msg.contains("Forwarding packet")) {
    // log.log("Adding FWD packet to network statistics\n");
    src_node = network.getNode(id);
    if (src_node == undefined) {
      src_node = network.addNode(id);
    }
    // log.log("Node: " + src_node.id + "\n");
    src_node.addForwardedPacket();
    // log.log("Forwarded packet ok\n");
  }
  if (msg.contains("[INFO: SA")) {
    log.log("SA message received\n");
    // Clear the number of messages received by the network
    network.clearStatistics();
    // Clear the number of messages received by each node
    network.clearNodesStatistics();
    // update the time
    time_aux = time;
  }
  if (msg.contains("[INFO: TSCH-LOG") && msg.contains("link  2  17") && msg.contains("rx") && msg.contains("len  30")) {
    src_node = network.getNode(id);
    if (src_node == undefined) {
      src_node = network.addNode(id);
    }
    src_node.addMacRxData();
  } else if (msg.contains("[INFO: TSCH-LOG") && msg.contains("link  2  17") && msg.contains("rx")) {
    src_node = network.getNode(id);
    if (src_node == undefined) {
      src_node = network.addNode(id);
    }
    src_node.addMacRxNA();
  }
  if (msg.contains("[INFO: TSCH-LOG") && msg.contains("link  2  17") && msg.contains("tx") && msg.contains("len  30")) {
    src_node = network.getNode(id);
    if (src_node == undefined) {
      src_node = network.addNode(id);
    }
    src_node.addMacTxData();
  } else if (msg.contains("[INFO: TSCH-LOG") && msg.contains("link  2  17") && msg.contains("tx")) {
    src_node = network.getNode(id);
    if (src_node == undefined) {
      src_node = network.addNode(id);
    }
    src_node.addMacTxNA();
  }
  // log.log("Checking if it is time to print statistics\n");
  time_difference = time - time_aux;
  if (time_difference > 60000000) {
    // log.log("Processing statistics\n");
    // print the time
    log.log(time_difference + "\n");
    time_aux = time;
    // Calculate the throughput for each node, iterate through the
    // nodes in the network object
    nodes_throughput_at_sink = network.calculateThroughputAtSink();
    // print the throughput for each node which are in a dictionary
    for (var key in nodes_throughput_at_sink) {
      log.log("Node " + key + " throughput at sink: " + nodes_throughput_at_sink[key] + "\n");
    }
    nodes_throughput_at_node = network.calculateThroughputAtNode();
    // print the throughput for each node which are in a dictionary
    for (var key in nodes_throughput_at_node) {
      log.log("Node " + key + " throughput at node: " + nodes_throughput_at_node[key] + "\n");
    }
    // Calculate the network throughput
    network_throughput = network.calculateNetworkThroughput();
    log.log("Network throughput: " + network_throughput + "\n");
    // Calculate the network throughput by summing the throughput of each node
    // calculateNetworkThroughputBySum();
    // print statistics per node
    network.printNodesStatistics();
    // Clear the number of messages received by the network
    network.clearStatistics();
    // Clear the number of messages received by each node
    network.clearNodesStatistics();
    log.log("--------------------------------------------------\n");
  }
}
