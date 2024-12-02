import { coord_to_angle } from "./projection";
import { UI } from "./ui";

const ui0 = new UI("ws://192.168.234.227/ws", "go_button0", "estop0", "code_input0", "status0");
const ui1 = new UI("ws://192.168.4.117/ws", "go_button1", "estop1", "code_input1", "status1");
const CANVAS_GO = document.getElementById("canvas_go") as HTMLButtonElement;

const canvas = document.getElementById("canvas") as HTMLCanvasElement;
canvas.width = canvas.clientWidth;
canvas.height = canvas.clientHeight
