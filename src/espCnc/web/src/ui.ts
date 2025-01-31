import { Command, CoolStepCommand, EnableCommand, EStopCommand, ExecutionSession, LineCommand, MicroCommand, MicrostepsCommand, MultiCommmand, SetupCommand, StartCommand, StealthChopCommand, StopCommand, TimedMoveCommand } from "./session";


function assert_len<T>(arr: Array<T>, len: number) {
    if(arr.length != len) throw new Error(`Bad length (${arr.length} != ${len})`);
}

function parse_command(cmd: string[]) : Command {
    if(cmd[0] == "start") {
        assert_len(cmd, 1);
        return new StartCommand();
    } else if(cmd[0] == "stop") {
        assert_len(cmd, 1);
        return new StopCommand();
    } else if(cmd[0] == "enable") {
        assert_len(cmd, 5);
        let nums = cmd.slice(1).map(Number)
        return new EnableCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if(cmd[0] == "micro" || cmd[0] == "microsteps") {
        assert_len(cmd, 5);
        let nums = cmd.slice(1).map(Number)
        return new MicrostepsCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if(cmd[0] == "stealth") {
        assert_len(cmd, 5);
        let nums = cmd.slice(1).map(Number)
        return new StealthChopCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if(cmd[0] == "cool") {
        assert_len(cmd, 5);
        let nums = cmd.slice(1).map(Number)
        return new CoolStepCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if(cmd[0] == "setup") {
        assert_len(cmd, 1);
        return new SetupCommand();
    } else if(cmd[0] == "test") {
        return test_timing();
    } else if(cmd[0] == "line") {
        assert_len(cmd, 3);
        let nums = cmd.slice(1).map(Number);
        return new LineCommand([nums[0], nums[1]]);
    } else {
        throw new Error("unknown " + cmd.toString());
    }
}

function position_function(t: number): number {
    return 0.1*Math.cos(t*16);
}

function test_timing(): Command {
    let cmds = []
    let x = 0
    let y = 0

    for(let i = 0; i < 40; i++) {
        let t = i / 4 * 2 * Math.PI;
        let ex = Math.sin(t) * 30
        let ey = Math.cos(t) * 30

        cmds.push(new LineCommand([ex-x,ey-y]))
        x = ex
        y = ey
    }
    cmds.push(new LineCommand([-x,-y]))
    return new MultiCommmand(cmds)
}

export class UI {
    ws: WebSocket;
    go_button: HTMLButtonElement;
    estop: HTMLButtonElement;
    code: HTMLTextAreaElement;
    status: HTMLDivElement;
    onMicros: ((cmds: MicroCommand[])=>void) = null;
    private waiting_promises: {n: number, res: ()=>void}[] = [];
    session: ExecutionSession;
    constructor(ws_url: string, go_id: string, estop_id: string, code_id: string, status_id: string){
        this.session = new ExecutionSession();

        this.code = document.getElementById(code_id) as HTMLTextAreaElement;
        this.go_button = document.getElementById(go_id) as HTMLButtonElement;
        this.estop = document.getElementById(estop_id) as HTMLButtonElement;
        this.status = document.getElementById(status_id) as HTMLDivElement;

        
        this.estop.addEventListener("click", ()=>{
            let cmds = [new EStopCommand()];
            this.execute(cmds);
        });
        this.go_button.addEventListener("click", ()=>{
            this.upload_code();
        }) 

        if(ws_url) {
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
                this.new_status(status_index);
            })
            this.ws.onopen = ()=>{
                console.log("WS opened");
                this.status.innerHTML = "CONNECTED";
            }
            this.ws.onclose = ()=>{
                console.log("WS closed");
                this.status.innerHTML = "DISCONNECTED";
            }
            this.ws.onerror = e=>{
                console.log(e);
                this.status.innerHTML = "ERROR";
            }
        }  
    }

    private new_status(status_index: number) {
        this.status.innerHTML = status_index+"";
        this.waiting_promises = this.waiting_promises.filter(x=>{
            let {n,res} = x;
            if(n == status_index) {
                res();
                return false;
            } else {
                return true;
            }
        })
    }

    execute(cmds: Command[]) {
        let micros = this.session.compile_commands(cmds, 0);
        let binary = this.session.serialize_micros(micros);
        if(this.onMicros) this.onMicros(micros);
        if(this.ws) this.ws.send(binary);
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
            commands.push(parse_command(cmd));
        }

        this.execute(commands);
    }

    async wait_for_status(status: number): Promise<void>{
        return new Promise((res,rej)=>{
            this.waiting_promises.push({n:status,res:res})
        })
    }
}