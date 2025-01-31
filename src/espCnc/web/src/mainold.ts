import { Preview } from "./preview";
import { UI } from "./ui";

// const ui0 = new UI("ws://192.168.234.227/ws", "go_button0", "estop0", "code_input0", "status0");
const ui0 = new UI("ws://192.168.234.193/ws", "go_button0", "estop0", "code_input0", "status0");
// const ui0 = new UI("ws://192.168.4.117/ws", "go_button0", "estop0", "code_input0", "status0");
// const ui1 = new UI("ws://192.168.4.117/ws", "go_button1", "estop1", "code_input1", "status1");
const canvas = document.getElementById("canvas") as HTMLCanvasElement;
canvas.width = canvas.clientWidth;
canvas.height = canvas.clientHeight
const preview = new Preview(canvas);

let start_time = 0;
let sim_time = 0.0;
function update_frame(t) {
    sim_time = (t - start_time) / 1000.0;
    preview.set_time(sim_time);
    requestAnimationFrame(update_frame);
}

(async ()=>{
    await preview.load_assets();
    update_frame(0);
    
    ui0.onMicros = micros=>{
        preview.set_program(micros);
        preview.set_time(0);
        sim_time = 0.0;
        start_time = performance.now();
    }
})()

let last_time = 0;
function animate(t: number) {
    let dt = t - last_time;
    last_time = t;
    requestAnimationFrame(animate);
    preview.update_camera(dt/1000);
    preview.draw();
}

animate(0);
