import { DelayCommand, ExecutionSession, StartCommand, StopCommand } from "./session";
import { UI } from "./ui";

const ui = new UI("ws://192.168.4.124/ws", "go_button0", "estop0", "code_input0", "status0");
// const ui = new UI("ws://192.168.234.193/ws", "go_button0", "estop0", "code_input0", "status0");

// const live_ports: SerialPort[] = [];
// let print_buff = "";

// const SerialOut = new WritableStream<Uint8Array>({
//     start(controller) { console.log(controller)},
//     write(chunk, controller) { 
//         let text = new TextDecoder().decode(chunk);
//         print_buff += text;
//         if(print_buff.endsWith("\n")) {
//             console.log(print_buff);
//             print_buff = "";
//         }
//     },
//     abort(reason) {console.log(reason)},
// })

// navigator.serial.getPorts().then((ports) => {
//     console.log(ports);
//     ports.forEach(register_port);
// });

// async function delay(millis: number) {
//     return new Promise((res,rej)=>{
//         setTimeout(res,millis)
//     })
// }

// async function register_port(port: SerialPort){
//     await port.open({baudRate:460800,bufferSize:1<<22});
//     port.readable.pipeTo(SerialOut);
//     await delay(200);
//     live_ports.push(port);
// }

// async function write_port(port: SerialPort, msg: Uint8Array) {
//     let writer = port.writable.getWriter();
//     await writer.ready;
//     console.log(writer.desiredSize);
//     console.time("send");
//     for(let i = 0; i < 100; i++){
//         writer.desiredSize
//         await writer.write(new Uint8Array(20));
//     }
//     console.timeEnd("send");

//     writer.releaseLock();
// }


// document.getElementById("run").addEventListener("click", ()=>{
//     // let micros = session.compile_commands(new Array(100).fill(new StopCommand()),1);
//     // let binary = session.serialize_micros(micros);
//     let binary = new Uint8Array(500);
//     // console.log(binary)
//     live_ports.forEach(async port=>{
//         await write_port(port, binary);
//     })
// });

// document.getElementById("connect").addEventListener("click", () => {
//     const usbVendorId = 0x303A;
//     navigator.serial
//         .requestPort({ filters: [{ usbVendorId }] })
//         .then((port) => {
//             register_port(port)
//         })
//         .catch((e) => {
//             console.log(e);
//         });
// });
