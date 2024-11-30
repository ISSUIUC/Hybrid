import { StopNowCommand, SpeedCommand, MoveCommand, StartCommand, StopCommand, EnableCommand, WaitCommand, MicrostepsCommand, ZeroCommand, MusicCommand, StealthChopCommand, CoolStepCommand, MusicModeCommand, XYCommand, LineCommand, CircleCommand, Command, MultiCommmand, StatusCommand, SetupCommand } from "./commands";
import { angle_to_coord, coord_to_angle } from "./projection";


function assert_len<T>(arr: Array<T>, len: number) {
    if(arr.length != len) throw new Error(`Bad length (${arr.length} != ${len})`);
}

function parse_command(cmd: string[]) : Command {
    if(cmd[0] == "speed") {
        assert_len(cmd, 2);
        return new SpeedCommand(eval(cmd[1]));
    } else if(cmd[0] == "move") {
        assert_len(cmd, 5);
        let nums = cmd.slice(1).map(eval)
        return new MoveCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if(cmd[0] == "start") {
        assert_len(cmd, 1);
        return new StartCommand();
    } else if(cmd[0] == "stop") {
        assert_len(cmd, 1);
        return new StopCommand();
    } else if(cmd[0] == "enable") {
        assert_len(cmd, 5);
        let nums = cmd.slice(1).map(Number)
        return new EnableCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if(cmd[0] == "wait") {
        assert_len(cmd, 2);
        return new WaitCommand(Number(cmd[1]) * 1000000 | 0)
    } else if(cmd[0] == "micro" || cmd[0] == "microsteps") {
        assert_len(cmd, 5);
        let nums = cmd.slice(1).map(Number)
        return new MicrostepsCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if(cmd[0] == "zero") {
        assert_len(cmd, 1);
        return new ZeroCommand();
    } else if(cmd[0] == "note") {
        return new MusicCommand(cmd[1], Number(cmd[2]));
    } else if(cmd[0] == "stealth") {
        assert_len(cmd, 5);
        let nums = cmd.slice(1).map(Number)
        return new StealthChopCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if(cmd[0] == "cool") {
        assert_len(cmd, 5);
        let nums = cmd.slice(1).map(Number)
        return new CoolStepCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if(cmd[0] == "musicmode") {
        assert_len(cmd, 1);
        return new MusicModeCommand();
    } else if(cmd[0] == "xy") {
        assert_len(cmd, 3);
        let nums = cmd.slice(1).map(Number)
        return new XYCommand([nums[0],nums[1]]);
    } else if(cmd[0] == "line") {
        assert_len(cmd, 5);
        let nums = cmd.slice(1).map(Number)
        return new LineCommand([nums[0],nums[1]],[nums[2],nums[3]]);
    } else if(cmd[0] == "circle") {
        assert_len(cmd, 4);
        let nums = cmd.slice(1).map(Number)
        return new CircleCommand(nums[0], [nums[1],nums[2]]);
    } else if(cmd[0] == "status") {
        assert_len(cmd, 2);
        return new StatusCommand(Number(cmd[1]));
    } else if(cmd[0] == "setup") {
        assert_len(cmd, 1);
        return new SetupCommand();
    } else {
        throw new Error("unknown " + cmd.toString());
    }
}
function apply_macro(macro: string[], stack: Command[]) {
    if(macro[0] == "#repeat") {
        assert_len(macro, 2);
        let last = stack.pop();
        let ct = Number(macro[1]);
        stack.push(new MultiCommmand(new Array(ct).fill(last)));
    }
}

export class UI {
    ws: WebSocket;
    go_button: HTMLButtonElement;
    estop: HTMLButtonElement;
    code: HTMLTextAreaElement;
    waitin_promises: {n: number, res: ()=>void}[] = [];
    constructor(ws_url: string, go_id: string, estop_id: string, code_id: string){
        this.ws = new WebSocket(ws_url);
        this.ws.onmessage = (ev=>{
            let s = window.atob(ev.data);
            let d = new Uint8Array(s.length);
            for(let i = 0; i < s.length; i++){
                let v = s.charCodeAt(i);
                d[i] = v;
            }
            let u32 = new Uint32Array(d.buffer);
            let status_index = u32[0];
            this.#new_status(status_index);
        })
        this.code = document.getElementById(code_id) as HTMLTextAreaElement;
        this.go_button = document.getElementById(go_id) as HTMLButtonElement;
        this.estop = document.getElementById(estop_id) as HTMLButtonElement;

        this.ws.onopen = ()=>{
            console.log("WS opened");
        }
        this.ws.onclose = ()=>{
            console.log("WS closed");
        }
        this.ws.onerror = console.log;
        
        this.estop.addEventListener("click", ()=>{
            this.ws.send(new Uint32Array(new StopNowCommand().encode()));
        });
        this.go_button.addEventListener("click", ()=>{
            this.upload_code()
        })   
    }

    execute(cmds: Command[]) {
        const binary = new Int32Array(cmds.flatMap(x=>x.encode()));
        this.ws.send(binary);
    }

    #new_status(status_index: number) {
        this.waitin_promises = this.waitin_promises.filter(x=>{
            let {n,res} = x;
            if(n == status_index) {
                res();
                return false;
            } else {
                return true;
            }
        })
    }

    upload_code() {
        const code = this.code.value;
        const lines = code.split('\n');
        const cmds = lines
                        .map(l=>l.toLowerCase())
                        .map(l=>l.split(/\s/).filter(x=>x))
                        .filter(x=>x.length);

        let commands = [];
        for(const cmd of cmds) {
            if(cmd[0].startsWith('#')) {
                apply_macro(cmd, commands);
            } else {
                commands.push(parse_command(cmd));
            }
        }
        this.execute(commands);
    }

    async wait_for_status(status: number): Promise<void>{
        return new Promise((res,rej)=>{
            this.waitin_promises.push({n:status,res:res})
        })
    }
}