export const CMD_TYPES = {
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
    index: number = 0;
    constructor(
        public command_type: number,
        public data?: Uint8Array,
    ){}
}

export type Command = {
    encode(): MicroCommand[];
}

export class NullCommand implements Command {
    encode(): MicroCommand[] {
        return []
    }
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

export class DelayCommand implements Command {
    constructor(public micros: number){}
    encode(): MicroCommand[] {
        return new TimedMoveCommand(new Uint8Array(this.micros)).encode()
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
        let buff = new Uint8Array(this.sequence.byteLength + 4);
        let u32 = new Uint32Array(buff.buffer, 0, 1);
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
            if(i - last_cmd_index == 255) {
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

        return new MultiCommmand(parts).encode()
    }
}

export class MultiCommmand implements Command {
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

//x = bit 2
//y = bit 3
//pencil = bit 1
export class LineCommand implements Command {
    constructor(public dst: [number,number,number,number]){}
    encode(): MicroCommand[] {
        // let steps = []
        // let pos = []

        // for (let i = 0; i < 1000000; i++){
        //     let t = Math.sqrt(i / 1e6) * 1000;
        //     let a = 10;
        //     let t1 = (i / 1e6)**2 * 1000;;
        //     let a1 = 10;
        //     pos.push(Math.sin(t*2*Math.PI)*a+Math.sin(t1*2*Math.PI)*a1)
        // }

        // for (let i = 0; i < pos.length - 1; i++){
        //     if(Math.floor(pos[i]) - Math.floor(pos[i+1]) > 0){
        //         steps.push(0xf);
        //     } else if (Math.floor(pos[i]) - Math.floor(pos[i+1]) < 0) {
        //         steps.push(0xff);
        //     } else {
        //         steps.push(0x00);
        //     }
        // }

        // return new TimedMoveCommand(new Uint8Array(steps)).encode()
        // 20 cm / rotation
        const micros = 8
        const step_per_mm = (micros * 200) / 8;
        // const step_per_mm = micros * 200 / 40;
        const f = 1.4
        // const max_speed = 140;
        // const max_accel = 4000;
        // const max_speed = 60;
        // const max_accel = 500;
        // const max_speed = 130;
        // const max_accel = 1500;
        const max_speed = 20;
        const max_accel = 100;
        
        let major_axis = this.dst.reduce((a,b)=>Math.max(Math.abs(a),Math.abs(b)))
        let accel_time = max_speed / max_accel;
        let accel_dist = accel_time**2 * max_accel / 2;

        let pos = 0;
        let dt = 1e-6;
        
        let steps = []
        let i = 0;
        let dist = 0;

        let calc_step = (pos,dpos)=>{
            let pos_steps = pos * step_per_mm;
            let next_pos_steps = (pos + dpos) * step_per_mm;
            let step = 0x00;
            for(let i = 0; i < 4; i++){
                if(Math.round(pos_steps * this.dst[i] / major_axis) < Math.round(next_pos_steps * this.dst[i] / major_axis)) {
                    step |= 0x01 << i;
                    dist += 1;
                }
                if(Math.round(pos_steps * this.dst[i] / major_axis) > Math.round(next_pos_steps * this.dst[i] / major_axis)) {
                    step |= 0x11 << i;
                    dist -= 1;
                }
            }
            return step;
        }

        for(; pos < major_axis / 2 && pos < accel_dist; i++) {
            let vel = i * dt * max_accel;
            steps.push(calc_step(pos, vel * dt));
            pos += vel * dt;
        }

        while(pos < major_axis - accel_dist) {
            let vel = i * dt * max_accel;
            steps.push(calc_step(pos, vel * dt));
            pos += vel * dt;
        }

        for(; pos < major_axis; i--) {
            let vel = i * dt * max_accel;
            steps.push(calc_step(pos, vel * dt));
            pos += vel * dt;
        }   
        console.log(dist)

        return new TimedMoveCommand(new Uint8Array(steps)).encode();
    }
}

export class WaitCommand implements Command{
    constructor(public t) {

    }
    encode(): MicroCommand[] {
        return new TimedMoveCommand(new Uint8Array(this.t * 1000000)).encode()
    }
}

export class ExecutionSession {
    constructor(){}
    compile_commands(commands: Command[], base_index: number): MicroCommand[] {
        let index = base_index;
        let micro_commands = [];

        for(const cmd of commands) {
            let micros = cmd.encode();
            for(const micro of micros) {
                micro.index = index;
                micro_commands.push(micro);
            }
            index ++;
        }

        return micro_commands;
    }

    serialize_micros(micros: MicroCommand[]): Uint8Array {
        let chunks = [];
        for(const micro of micros) {
            let chunk = new Uint8Array(4 + (micro.data?.length | 0));
            let u16 = new Uint16Array(chunk.buffer);
            u16[0] = micro.command_type;
            u16[1] = micro.index;
            if(micro.data){
                chunk.set(micro.data, 4);
            }
            chunks.push(chunk);
        }

        return concat(chunks);
    }
}