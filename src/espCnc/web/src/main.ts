const CONNECT = document.getElementById("connect");
const SEND = document.getElementById("send");
const CODE = document.getElementById("code") as HTMLTextAreaElement;
const STATUS = document.getElementById("status");

let bytes_in_flight = 0;
let send_queue = "";
let bytes_sent = 0;

async function read_task(reader: ReadableStreamDefaultReader<Uint8Array>) {
    const decoder = new TextDecoder();
    while(true) {
        const {value, done} = await reader.read();
        if(done) {
            console.log("Reader Done");
            return;
        }

        const str_value = decoder.decode(value);
        console.log(str_value);
        for(const c of str_value) {
            if(c == '%') bytes_in_flight -= 1;
        }
    }
}

async function write_task(writer: WritableStreamDefaultWriter<Uint8Array>) {
  const encoder = new TextEncoder();
    setInterval(async ()=>{
        STATUS.innerHTML = `${send_queue.length} ${bytes_sent} ${bytes_in_flight}`;
        if(bytes_in_flight < 64 && send_queue.length != 0) {
            const next_chunk = send_queue.slice(0,128);
            send_queue = send_queue.slice(128)
            const val = encoder.encode(next_chunk);
            console.log(next_chunk);
            writer.write(val);
            bytes_in_flight += val.byteLength;
            bytes_sent += val.byteLength;
        }
    });
}

CONNECT.addEventListener("click", async () => {
  const port = await navigator.serial.requestPort()
  await port.open({baudRate: 460800});
  const writer = await port.writable.getWriter();
  const reader = await port.readable.getReader();
  read_task(reader);
  write_task(writer);
});

SEND.addEventListener("click", () => {
    send_queue += CODE.value;
});