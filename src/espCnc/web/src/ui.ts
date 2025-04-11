import { Command, CoolStepCommand, EnableCommand, EStopCommand, ExecutionSession, LineCommand, MicroCommand, MicrostepsCommand, MultiCommmand, NullCommand, SetupCommand, StartCommand, StealthChopCommand, StopCommand, TimedMoveCommand, WaitCommand } from "./session";


function assert_len<T>(arr: Array<T>, len: number) {
    if(arr.length != len) throw new Error(`Bad length (${arr.length} != ${len})`);
}

async function parse_command(cmd: string[]) : Promise<Command> {
    if(cmd[0].startsWith("#")) return new NullCommand();
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
    } else if(cmd[0] == "line") {
        let nums = cmd.slice(1).map(Number);
        if(nums.length == 4){
            return new LineCommand([nums[0], nums[1], nums[2], nums[3]]);
        }
        if(nums.length == 1){
            return new LineCommand([nums[0],nums[0],nums[0],nums[0]]);
        }
    } else if(cmd[0] == "wait") {
        assert_len(cmd, 2);
        let t = Number(cmd[1])
        return new WaitCommand(t);
    } else if(cmd[0] == "play") {
        assert_len(cmd, 2);
        let file_name = cmd[1];
        let buff = await fetch(file_name).then(b=>b.arrayBuffer());
        return new TimedMoveCommand(new Uint8Array(buff));
    } else {
        throw new Error("unknown " + cmd.toString());
    }
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
        this.go_button.addEventListener("click", async ()=>{
            await this.upload_code();
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
        console.log("Binary size", binary.length)
        if(this.ws) this.ws.send(binary);
    }

    async upload_code() {
        const code = this.code.value;
        const lines = code.split('\n');
        const cmds = lines
                        .map(l=>l.toLowerCase())
                        .map(l=>l.split(/\s/).filter(x=>x))
                        .filter(x=>x.length);

        let commands = [];
        for(const cmd of cmds) {
            commands.push(await parse_command(cmd));
        }

        this.execute(commands);
    }

    async wait_for_status(status: number): Promise<void>{
        return new Promise((res,rej)=>{
            this.waiting_promises.push({n:status,res:res})
        })
    }
}