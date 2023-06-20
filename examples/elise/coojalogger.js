/* A simple log file generator script */

// TIMEOUT(144000000); /* 3600 seconds or 1 hour */
TIMEOUT(1800000); /* 3600 seconds or 1 hour */

packet_size = 11;
sample_time = 60; // 1 minute

time_aux = 0;
time_difference = 0;
network_rx = 0;
network_throughput = 0;
network_throughput_list = [];
// Create a struct that holds the throughput values for each node in the network
function Throughput(node_id) {
  this.node_id = node_id;
  this.num_data_pkts = 0;
  this.num_na_pkts = 0;
  this.last_throughput = 0;
  this.all_throughput = [];
  this.average_throughput = 0;
}
// Create a linked list to hold the routing table
function LinkedList() {
  this.head = null;
  this.tail = null;
}
// Create a node to hold the routing table entry
function Route(node_id, dst_id, via) {
  this.node_id = node_id;
  this.dst_id = dst_id;
  this.via = via;
  this.next = null;
}
// Add a routing table entry to the linked list
LinkedList.prototype.addRoute = function (node_id, dst_id, via) {
  var node = new Route(node_id, dst_id, via);
  // If the list is empty, add the node as the head and tail
  if (this.head == null) {
    this.head = node;
    this.tail = node;
  } else {
    // If the list is not empty, add the node to the tail
    this.tail.next = node;
    this.tail = node;
  }
};
// Use the addRoute method to add a routing table entry to the linked list
var routing_table = new LinkedList();
routing_table.addRoute(2, 1, 1);
routing_table.addRoute(3, 1, 1);
routing_table.addRoute(4, 1, 1);
routing_table.addRoute(5, 1, 3);
// print the routing table
log.log("Routing table\n");
var current = routing_table.head;
while (current != null) {
  log.log(
    "Node " +
    current.node_id +
    " dst_id: " +
    current.dst_id +
    " via: " +
    current.via +
    "\n"
  );
  current = current.next;
}
// Loop through the path from 5 to 1 and print the path
log.log("Path from 5 to 1\n");
// Get the node 5 entry
var current = routing_table.head;
while (current != null) {
  if (current.node_id == 5) {
    break;
  }
  current = current.next;
}
// Print the path
while (current != null) {
  log.log(
    "Node " +
    current.node_id +
    " dst_id: " +
    current.dst_id +
    " via: " +
    current.via +
    "\n"
  );
  current = current.via;
  log.log("current: " + current + "\n");
}
// List to append the throughput values
throughput_list = [];
log.log("Starting COOJA logger\n");

timeout_function = function () {
  log.testOK();
};

// Throughput calculation based on the number of messages received
function calculateThroughput(num_pkts) {
  // Calculate the throughput
  throughput = num_pkts * packet_size * 8 / sample_time;
  return throughput;
}

function average(list) {
  sum = 0;
  for (i = 0; i < list.length; i++) {
    sum += list[i];
  }
  average_list = sum / list.length;
  return average_list;
}

// Method to add num rx messages count to the node
function addRxPacket(node_id, num_pkts, field) {
  // Check if the node is already in the list
  for (i = 0; i < throughput_list.length; i++) {
    if (throughput_list[i].node_id == node_id) {
      // Add the number of packets received to the specified field of the node
      throughput_list[i][field] += num_pkts;
      return;
    }
  }
  // If the node is not in the list, add it
  var newThroughput = new Throughput(node_id);
  newThroughput[field] = num_pkts;
  throughput_list.push(newThroughput);
}


// Calculate the throughput for each node
function calculateThroughputByNode() {
  // Calculate the throughput for each node
  var i;
  for (i = 0; i < throughput_list.length; i++) {
    // Calculate the last throughput
    throughput_list[i].last_throughput = calculateThroughput(
      throughput_list[i].num_data_pkts + throughput_list[i].num_na_pkts);
    log.log(
      "Node " +
      throughput_list[i].node_id +
      " num_data_pkts: " +
      throughput_list[i].num_data_pkts +
      " num_na_pkts: " +
      throughput_list[i].num_na_pkts +
      " last_throughput: " +
      throughput_list[i].last_throughput +
      "\n"
    );
    // Add the last throughput to the list of all throughput values
    throughput_list[i].all_throughput.push(
      throughput_list[i].last_throughput
    );
    // Calculate the average throughput
    throughput_list[i].average_throughput = average(
      throughput_list[i].all_throughput
    );
    log.log(
      "Node " +
      throughput_list[i].node_id +
      " average_throughput: " +
      throughput_list[i].average_throughput +
      "\n"
    );
    // Reset the number of messages received
    throughput_list[i].num_data_pkts = 0;
    throughput_list[i].num_na_pkts = 0;
  }
}

// Calculate the network throughput
function calculateNetworkThroughput() {
  // Calculate the network throughput
  network_throughput = calculateThroughput(network_rx);
  log.log("Network throughput: " + network_throughput + "\n");
  // Add the network throughput to the list of all network throughput values
  network_throughput_list.push(network_throughput);
  // Calculate the average network throughput
  average_network_throughput = average(network_throughput_list);
  log.log("Average network throughput: " + average_network_throughput + "\n");
  // Reset the network number of messages received
  network_rx = 0;
}

// Calculate the network throughput by summing the throughput of each node
function calculateNetworkThroughputBySum() {
  // Calculate the network throughput
  average_network_throughput = average(network_throughput_list);
  log.log("Network based on individual nodes throughput: " + network_throughput + "\n");
}

while (true) {
  YIELD();
  if (msg.contains("Data message received from ")) {
    // Network number of messages received
    network_rx += 1;
    // Get the node id, which is at the end of the message
    node_id = msg.slice(-1);
    // Add the number of messages received to the node
    addRxPacket(node_id, 1, "num_data_pkts");
  }
  if (msg.contains("NA message received from ")) {
    // Network number of messages received
    network_rx += 1;
    // Get the node id, which is at the end of the message
    node_id = msg.slice(-1);
    // Add the number of messages received to the node
    addRxPacket(node_id, 1, "num_na_pkts");
  }
  if (msg.contains("[INFO: SA")) {
    // Clear the number of messages received by each node
    throughput_list = [];
    // Reset the time
    time_aux = time;
    // Reset the network number of messages received
    network_rx = 0;
    // Reset the network throughput list
    network_throughput_list = [];
  }
  time_difference = time - time_aux;
  if (time_difference > 60000000) {
    // print the time
    log.log(time_difference + "\n");
    time_aux = time;
    // Calculate the throughput for each node
    calculateThroughputByNode();
    // Calculate the network throughput
    calculateNetworkThroughput();
    // Calculate the network throughput by summing the throughput of each node
    calculateNetworkThroughputBySum();
  }
}
