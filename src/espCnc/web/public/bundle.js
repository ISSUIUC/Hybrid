(() => {
  // src/main.ts
  var CONNECT = document.getElementById("connect");
  var SEND = document.getElementById("send");
  var CODE = document.getElementById("code");
  var STATUS = document.getElementById("status");
  var bytes_in_flight = 0;
  var send_queue = "";
  var bytes_sent = 0;
  var print_buff = "";
  async function read_task(reader) {
    const decoder = new TextDecoder();
    while (true) {
      const { value, done } = await reader.read();
      if (done) {
        console.log("Reader Done");
        return;
      }
      const str_value = decoder.decode(value);
      for (let c of str_value) {
        if (c == "%") {
          bytes_in_flight--;
        } else if (c == "\n") {
          console.log(print_buff);
          print_buff = "";
        } else {
          print_buff += c;
        }
      }
    }
  }
  async function write_task(writer) {
    const encoder = new TextEncoder();
    setInterval(async () => {
      STATUS.innerHTML = `${send_queue.length} ${bytes_sent} ${bytes_in_flight}`;
      if (bytes_in_flight < 64 && send_queue.length != 0) {
        const next_chunk = send_queue.slice(0, 128);
        send_queue = send_queue.slice(128);
        const val = encoder.encode(next_chunk);
        writer.write(val);
        bytes_in_flight += val.byteLength;
        bytes_sent += val.byteLength;
      }
    });
  }
  CONNECT.addEventListener("click", async () => {
    const port = await navigator.serial.requestPort();
    await port.open({ baudRate: 460800 });
    const writer = await port.writable.getWriter();
    const reader = await port.readable.getReader();
    read_task(reader);
    write_task(writer);
  });
  SEND.addEventListener("click", () => {
    send_queue += CODE.value;
  });
})();
//# sourceMappingURL=bundle.js.map
