import { LineCommand, MoveCommand, StatusCommand, XYCommand } from "./commands";
import { coord_to_angle } from "./projection";
import { UI } from "./ui";

const ui0 = new UI("ws://192.168.234.227/ws", "go_button0", "estop0", "code_input0");
const ui1 = new UI("ws://192.168.234.216/ws", "go_button1", "estop1", "code_input1");
const CANVAS_GO = document.getElementById("canvas_go") as HTMLButtonElement;

const canvas = document.getElementById("canvas") as HTMLCanvasElement;
canvas.width = canvas.clientWidth;
canvas.height = canvas.clientHeight
const ctx = canvas.getContext("2d");


let mouse = null;
function map_coords(xy: [number,number]){
    return [-(xy[0] / 100), xy[1] / 100 + 6]
}

let lines = []

canvas.addEventListener("mousedown", ev=>{
    const xy: [number,number] = [ev.clientX-canvas.offsetLeft,ev.clientY-canvas.offsetTop];
    mouse = xy;
})

canvas.addEventListener("mousemove", ev=>{
    if(!mouse) return;
    const xy: [number,number] = [ev.clientX-canvas.offsetLeft,ev.clientY-canvas.offsetTop];
    let next_mouse = xy;
    ctx.moveTo(mouse[0],mouse[1]);
    ctx.lineTo(next_mouse[0],next_mouse[1]);
    ctx.stroke();
    let m1 = map_coords(mouse);
    let m2 = map_coords(next_mouse)
    mouse = next_mouse;
    lines.push([m1,m2])
})

canvas.addEventListener("mouseup", ev=>{
    mouse = null;
})

window.addEventListener("keypress", ev=>{
    if(ev.key == ' '){
        mouse = null;
    }
})

function pt_eq(a:[number,number],b:[number,number]): boolean{
    return a[0]==b[0] &&a[1]==b[1];
}


CANVAS_GO.addEventListener("click", ev=>{
    let all_steps: [[number,number],[number,number]][][] = []
    let step = []
    let current_pos:[number,number] = [-1,-1]
    for(const line of lines) {
        if(pt_eq(line[0], current_pos)) {
            step.push(line);
        } else {
            all_steps.push(step);
            step = [line];
        }
        current_pos = line[1];
    }
    all_steps.push(step);
    all_steps = all_steps.filter(x=>x.length!=0)
    execute_lines(all_steps);
});

async function pencil_up(): Promise<void>{
    ui1.execute([new MoveCommand([1300,0,0,0]),new StatusCommand(101)])
    await ui1.wait_for_status(101);
    
}

async function pencil_down() {
    ui1.execute([new MoveCommand([1150,0,0,0]),new StatusCommand(102)])
    await ui1.wait_for_status(102);
}

let global_counter = 1000;
async function draw_lines(lines: [[number,number],[number,number]][]) {
    let n = global_counter++;
    let cmds = []
    for(const line of lines){
        cmds.push(new LineCommand(line[0],line[1]))
    }
    ui0.execute([...cmds, new StatusCommand(n)])
    await ui0.wait_for_status(n);
}

async function move_to(xy: [number,number]) {
    let n = global_counter++;
    ui0.execute([new XYCommand(xy), new StatusCommand(n)])
    await ui0.wait_for_status(n);
}

async function execute_lines(steps: [[number,number],[number,number]][][]){
    for(const step of steps) {
        await pencil_up();
        await move_to(step[0][0])
        await pencil_down();
        await draw_lines(step);
    }
    await pencil_up();
}
