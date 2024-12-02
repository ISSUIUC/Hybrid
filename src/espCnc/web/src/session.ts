const CMD_TYPES = {
    StopNow: 0,
    StopAll: 1,
    StartAll: 2,
    Enable: 3,
    SetMicrosteps: 4,
    StealthChop: 11,
    CoolStep: 12,
    SetStatus: 13,
    NewTimingReference: 14,
    Timed: 15,
}

const MAX_CHUNK = 4096;

function concat(buffs: Uint8Array[]) {
    const len = buffs.reduce((a,b)=>a + b.length, 0);
    const together = new Uint8Array(len)
    let head = 0;
    for(const buff of buffs) {
        together.set(buff, head);
        head += buff.length;
    }
    return together;
}

export class MicroCommand{
    constructor(
        public command_type: number,
        public data?: Uint8Array,
    ){}
}

export type Command = {
    encode(): MicroCommand[];
}

export class EStopCommand implements Command {
    encode(): MicroCommand[] {
        return [new MicroCommand(CMD_TYPES.StopNow)]
    }
}

export class StopCommand implements Command {
    encode(): MicroCommand[] {
        return [new MicroCommand(CMD_TYPES.StopAll)];
    }
}

export class StartCommand implements Command {
    encode(): MicroCommand[] {
        return [new MicroCommand(CMD_TYPES.StartAll)];
    }
}

export class MicrostepsCommand implements Command {
    constructor(public microsteps: [number,number,number,number]){}
    encode(): MicroCommand[] {
        return [new MicroCommand(CMD_TYPES.SetMicrosteps, new Uint8Array(new Uint16Array(this.microsteps).buffer))];
    }
}

export class StealthChopCommand implements Command {
    constructor(public enable: [number,number,number,number]){}
    encode(): MicroCommand[] {
        return [new MicroCommand(CMD_TYPES.StealthChop, new Uint8Array(this.enable))];
    }
}

export class CoolStepCommand implements Command {
    constructor(public enable: [number,number,number,number]){}
    encode(): MicroCommand[] {
        return [new MicroCommand(CMD_TYPES.CoolStep, new Uint8Array(this.enable))];
    }
}

export class EnableCommand implements Command {
    constructor(public enable: [number,number,number,number]){}
    encode(): MicroCommand[] {
        return [new MicroCommand(CMD_TYPES.Enable, new Uint8Array(this.enable))];
    }
}

class NewTimingReferenceCommand implements Command {
    constructor(){}
    encode(): MicroCommand[] {
        return [new MicroCommand(CMD_TYPES.NewTimingReference)];
    }
}

class ContinuousTimedSequenceCommand implements Command {
    constructor(public sequence: Uint16Array){
        if(sequence.length > MAX_CHUNK) throw new Error("Too big chunk");
    }
    encode(): MicroCommand[] {
        console.log(this.sequence);
        let buff = new Uint8Array(this.sequence.byteLength + 4);
        let u32 = new Uint32Array(buff.buffer);
        u32[0] = this.sequence.length;
        buff.set(new Uint8Array(this.sequence.buffer, this.sequence.byteOffset, this.sequence.byteLength), 4);

        return [new MicroCommand(CMD_TYPES.Timed, buff)];
    }
}

export class TimedMoveCommand implements Command {
    constructor(public step_sequence: Uint8Array) {}
    private encode_timings(): Uint16Array {
        let last_cmd_index = 0;
        let timing_commands = [];
        for(let i = 0; i < this.step_sequence.length; i++) {
            if(this.step_sequence[i] != 0) {
                timing_commands.push((i - last_cmd_index) | (this.step_sequence[i] << 8));
                last_cmd_index = i;
            }
            if(i - last_cmd_index == 256) {
                timing_commands.push((i - last_cmd_index) | 0x00);
                last_cmd_index = i;
            }   
        }
        timing_commands.push((this.step_sequence.length - last_cmd_index))
        return new Uint16Array(timing_commands);
    }
    encode(): MicroCommand[] {
        let commands = this.encode_timings();
        let parts = [new NewTimingReferenceCommand()];

        let chunk_ct = Math.ceil(commands.length / MAX_CHUNK);
        for(let i = 0; i < chunk_ct; i++) {
            let chunk = commands.subarray(i * MAX_CHUNK, (i+1)*MAX_CHUNK);
            parts.push(new ContinuousTimedSequenceCommand(chunk))
        }

        console.log(parts);

        return new MultiCommmand(parts).encode()
    }
}

class MultiCommmand implements Command {
    constructor(public commands: Command[]){}
    encode(): MicroCommand[] {
        return this.commands.flatMap(cmd=>cmd.encode());
    }
}

export class SetupCommand implements Command {
    encode(): MicroCommand[] {
        return new MultiCommmand([
            new StartCommand(),
            new MicrostepsCommand([256, 256, 256, 256]),
            new StealthChopCommand([1,1,1,1]),
            new CoolStepCommand([1,1,1,1]),
            new EnableCommand([1,1,1,1]),
        ]).encode()
    }
}

export class ExecutionSession {
    private cmd_index: number = 0;
    constructor(private ws: WebSocket){}
    private build_execution(commands: Command[], base_index: number): Uint8Array {
        let chunks = [];
        let index = base_index;
        for(const cmd of commands) {
            let micros = cmd.encode();
            for(const micro of micros) {
                let chunk = new Uint8Array(4 + (micro.data?.length | 0));
                let u16 = new Uint16Array(chunk.buffer);
                u16[0] = micro.command_type;
                u16[1] = index;
                if(micro.data){
                    chunk.set(micro.data, 4);
                }
                chunks.push(chunk);
            }
            index ++;
        }

        return concat(chunks);
    }
    execute(commands: Command[]) {
        const binary = this.build_execution(commands, this.cmd_index);
        this.cmd_index += commands.length;
        console.log(binary);
        this.ws.send(binary);
    }
}