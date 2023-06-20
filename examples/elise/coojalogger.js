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
  this.num_messages_rx = 0;
  this.last_throughput = 0;
  this.all_throughput = [];
  this.average_throughput = 0;
}
// List to append the throughput values
throughput_list = [];
log.log("Starting COOJA logger\n");

timeout_function = function () {
  log.testOK();
};

// Throughput calculation based on the number of messages received
function calculateThroughput(num_messages_rx) {
  // Calculate the throughput
  throughput = num_messages_rx * packet_size * 8 / sample_time;
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
function addRxPacket(node_id, num_messages_rx) {
  // Check if the node is already in the list
  for (i = 0; i < throughput_list.length; i++) {
    if (throughput_list[i].node_id == node_id) {
      // Add the number of messages received to the node
      throughput_list[i].num_messages_rx += num_messages_rx;
      return;
    }
  }
  // If the node is not in the list, add it
  throughput_list.push(new Throughput(node_id, num_messages_rx));
}

// Calculate the throughput for each node
function calculateThroughputByNode() {
  // Calculate the throughput for each node
  var i;
  for (i = 0; i < throughput_list.length; i++) {
    // Calculate the last throughput
    throughput_list[i].last_throughput = calculateThroughput(
      throughput_list[i].num_messages_rx);
    log.log(
      "Node " +
      throughput_list[i].node_id +
      " num_messages_rx: " +
      throughput_list[i].num_messages_rx +
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
    throughput_list[i].num_messages_rx = 0;
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
  if (msg.contains("Message received from ")) {
    // Network number of messages received
    network_rx += 1;
    // Get the node id, which is at the end of the message
    node_id = msg.slice(-1);
    // Add the number of messages received to the node
    addRxPacket(node_id, 1);
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
