/* A simple log file generator script */

// TIMEOUT(144000000); /* 3600 seconds or 1 hour */
TIMEOUT(1200000); /* 3600 seconds or 1 hour */

time_aux = 0;
time_difference = 0;
num_messages_rx = 0;
average_throughput = 0;
// List to append the throughput values
throughput_list = [];
log.log("Starting COOJA logger\n");

timeout_function = function () {
  sum_throughput = throughput_list.reduce(function (a, b) {
    return a + b;
  }, 0);
  log.log("Sum of throughput values: " + sum_throughput + "\n");
  average_throughput = sum_throughput / throughput_list.length;
  log.log("Average throughput: " + average_throughput + "\n");
  log.log("Script timed out.\n");
  log.testOK();
};

while (true) {
  YIELD();
  //   if (msg) {
  //       log.log(time + " " + id + " " + msg + "\n");
  //   }
  if (msg.contains("Message received")) {
    num_messages_rx += 1;
  }
  if (msg.contains("[INFO: SA")) {
    num_messages_rx = 0;
    // Clear the list
    throughput_list = [];
  }
  time_difference = time - time_aux;
  if (time_difference > 60000000) {
    // print the time
    log.log(time_difference + "\n");
    time_aux = time;
    log.log("number of packets received: " + num_messages_rx + "\n");
    // Throughput
    throughput = num_messages_rx / (time_difference / 60000000);
    log.log("Throughput: " + throughput + "\n");
    // Append the throughput value to the list
    throughput_list.push(throughput);
    // Reset the number of messages received
    num_messages_rx = 0;
  }
}
