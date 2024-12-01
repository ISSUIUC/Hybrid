import { add, angle_to_coord, coord_to_angle, length, mul, rotate, sub, Vec2 } from "./projection";

const CMD_TYPES = {
    StopNow: 0,
    StopAll: 1,
    StartAll: 2,
    Enable: 3,
    SetMicrosteps: 4,
    SetVelocity: 5,
    MoveTo: 6,
    SetStepDir: 7,
    SetSpeed: 8,
    Wait: 9,
    ZeroPosition: 10,
    StealthChop: 11,
    CoolStep: 12,
    SetStatus: 13,
    NewTimingReference: 14,
    Timed: 15,
};

export function concat(buffs: Uint8Array[]) {
    const len = buffs.reduce((a,b)=>a + b.length, 0);
    const together = new Uint8Array(len)
    let head = 0;
    for(const buff of buffs) {
        together.set(buff, head);
        head += buff.length;
    }
    return together;
}

function u32_from_array(arr: number[]) {
    return new Uint8Array(new Uint32Array(arr).buffer)
}

export interface Command {
    encode(): Uint8Array;
}

export class StopNowCommand implements Command {
    constructor(){}
    encode(): Uint8Array {
        return u32_from_array([CMD_TYPES.StopNow,0,0,0,0]);
    }
}

export class StartCommand implements Command {
    constructor(){}
    encode(): Uint8Array {
        return u32_from_array([CMD_TYPES.StartAll,0,0,0,0]);
    }
}

export class SetupCommand implements Command {
    constructor(){}
    encode(): Uint8Array {
        return new MultiCommmand([
            new StartCommand(),
            new MicrostepsCommand([256, 256, 256, 256]),
            new StealthChopCommand([1,1,1,1]),
            new CoolStepCommand([1,1,1,1]),
            new EnableCommand([1,1,1,1]),
        ]).encode();
    }
}

export class StopCommand implements Command {
    constructor(){}
    encode(): Uint8Array {
        return u32_from_array([CMD_TYPES.StopAll,0,0,0,0]);
    }
}

export class SpeedCommand implements Command {
    constructor(public delay_us: number){}
    encode(): Uint8Array {
        return u32_from_array([CMD_TYPES.SetSpeed,this.delay_us,0,0,0]);
    }
}

export class MoveCommand implements Command {
    constructor(public positions: [number,number,number,number]){}
    encode(): Uint8Array {
        return u32_from_array([CMD_TYPES.MoveTo, ...this.positions]);
    }
}

export class EnableCommand implements Command {
    constructor(public enables: [number,number,number,number]){}
    encode(): Uint8Array {
        return u32_from_array([CMD_TYPES.Enable, ...this.enables]);
    }
}

export class WaitCommand implements Command {
    constructor(public delay_us: number){}
    encode(): Uint8Array {
        return u32_from_array([CMD_TYPES.Wait, this.delay_us, 0, 0, 0]);
    }
}

export class MicrostepsCommand implements Command {
    constructor(public microsteps: [number,number,number,number]){}
    encode(): Uint8Array {
        return u32_from_array([CMD_TYPES.SetMicrosteps, ...this.microsteps]);
    }
}

export class ZeroCommand implements Command {
    constructor(){}
    encode(): Uint8Array {    
        return u32_from_array([CMD_TYPES.ZeroPosition, 0,0,0,0]);
    }
}

export class StealthChopCommand implements Command {
    constructor(public enables: [number,number,number,number]){}
    encode(): Uint8Array {
        return u32_from_array([CMD_TYPES.StealthChop, ...this.enables]);
    }
}

export class CoolStepCommand implements Command {
    constructor(public enables: [number,number,number,number]){}
    encode(): Uint8Array {
        return u32_from_array([CMD_TYPES.CoolStep, ...this.enables]);
    }
}

export class AbsoluteAngleCommand implements Command{
    constructor(public angles: [number,number,number,number]) {}
    encode(): Uint8Array {
        const full_rotation = 200*256;
        return new MoveCommand([
            this.angles[0] / Math.PI / 2 * full_rotation,
            this.angles[1] / Math.PI / 2 * full_rotation,
            this.angles[2] / Math.PI / 2 * full_rotation,
            this.angles[3] / Math.PI / 2 * full_rotation,
        ]).encode()
    }
}

export class XYCommand implements Command {
    constructor(public xy: Vec2){}
    encode(): Uint8Array {
        let coords = coord_to_angle(this.xy);
        return new AbsoluteAngleCommand([...coords, 0, 0]).encode();
    }
}

export class LineCommand implements Command {
    constructor(public start: Vec2, public stop: Vec2){}
    encode(): Uint8Array {
        let dist = length(sub(this.start,this.stop))
        let substeps = Math.ceil(dist * 40)
        let cmds = []
        for(let i = 0; i < substeps; i++){
            let xy = add(mul(this.start, 1 - (i/substeps)), mul(this.stop, (i/substeps)));
            cmds.push(new XYCommand(xy));
        }
        cmds.push(new XYCommand(this.stop))
        return concat(cmds.map(cmd=>cmd.encode()));
    }
}

export class CircleCommand implements Command {
    constructor(public r: number, public center: Vec2) {}
    encode(): Uint8Array {
        let substeps = Math.ceil(this.r * 2 * Math.PI * 20);
        let cmds = [];
        for(let i = 0; i <= substeps; i++){
            let theta = i / substeps * Math.PI * 2;
            let xy = add(this.center, rotate([this.r,0], theta));
            cmds.push(new XYCommand(xy))
        }
        return concat(cmds.map(cmd=>cmd.encode()));
    }
}

export class StatusCommand implements Command {
    constructor(public status: number) {}
    encode(): Uint8Array {
        return u32_from_array([CMD_TYPES.SetStatus, this.status, 0, 0, 0]);
    }
}

export class MultiCommmand implements Command {
    constructor(public cmds: Command[]) {}
    encode(): Uint8Array {
        return concat(this.cmds.map(cmd=>cmd.encode()));
    }
}

export class TimedMoveCommand implements Command {
    constructor(public step_sequence: Uint8Array) {}

    encode_timings(): Uint16Array {
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

    encode(): Uint8Array {
        let commands = this.encode_timings();

        let header = [
            CMD_TYPES.NewTimingReference, 0, 0, 0, 0,
            CMD_TYPES.Timed, commands.length, 0, 0, 0,
        ]

        return concat([
            u32_from_array(header),
            new Uint8Array(commands.buffer),
        ])
    }
}