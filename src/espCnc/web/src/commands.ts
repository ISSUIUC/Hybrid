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
    Zero: 10,
    StealthChop: 11,
    CoolStep: 12,
    Status: 13,
}

export interface Command {
    encode(): number[];
}

export class StopNowCommand implements Command {
    constructor(){}
    encode(): number[] {
        return [CMD_TYPES.StopNow,0,0,0,0];
    }
}

export class StartCommand implements Command {
    constructor(){}
    encode(): number[] {
        return [CMD_TYPES.StartAll,0,0,0,0];
    }
}

export class SetupCommand implements Command {
    constructor(){}
    encode(): number[] {
        return [
            ...new MicrostepsCommand([256, 256, 256, 256]).encode(),
            ...new StealthChopCommand([1,1,1,1]).encode(),
            ...new CoolStepCommand([1,1,1,1]).encode(),
        ]
    }
}

export class StopCommand implements Command {
    constructor(){}
    encode(): number[] {
        return [CMD_TYPES.StopAll,0,0,0,0];
    }
}

export class SpeedCommand implements Command {
    constructor(public delay_us: number){}
    encode(): number[] {
        return [CMD_TYPES.SetSpeed,this.delay_us,0,0,0];
    }
}

export class MoveCommand implements Command {
    constructor(public positions: [number,number,number,number]){}
    encode(): number[] {
        return [CMD_TYPES.MoveTo, ...this.positions];
    }
}

export class EnableCommand implements Command {
    constructor(public enables: [number,number,number,number]){}
    encode(): number[] {
        return [CMD_TYPES.Enable, ...this.enables];
    }
}

export class WaitCommand implements Command {
    constructor(public delay_us: number){}
    encode(): number[] {
        return [CMD_TYPES.Wait, this.delay_us, 0, 0, 0];
    }
}

export class MicrostepsCommand implements Command {
    constructor(public microsteps: [number,number,number,number]){}
    encode(): number[] {
        return [CMD_TYPES.SetMicrosteps, ...this.microsteps];
    }
}

export class ZeroCommand implements Command {
    constructor(){}
    encode(): number[] {    
        return [CMD_TYPES.Zero, 0,0,0,0];
    }
}

export class StealthChopCommand implements Command {
    constructor(public enables: [number,number,number,number]){}
    encode(): number[] {
        return [CMD_TYPES.StealthChop, ...this.enables];
    }
}

export class CoolStepCommand implements Command {
    constructor(public enables: [number,number,number,number]){}
    encode(): number[] {
        return [CMD_TYPES.CoolStep, ...this.enables];
    }
}

export class MusicModeCommand implements Command {
    constructor(){}
    encode(): number[] {
        return [
            ...new MicrostepsCommand([1,1,1,1]).encode(),
            ...new StealthChopCommand([0,0,0,0]).encode(),
        ]
    }
}

export class AbsoluteAngleCommand implements Command{
    constructor(public angles: [number,number,number,number]) {}
    encode(): number[] {
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
    encode(): number[] {
        let coords = coord_to_angle(this.xy);
        return new AbsoluteAngleCommand([...coords, 0, 0]).encode();
    }
}

export class LineCommand implements Command {
    constructor(public start: Vec2, public stop: Vec2){}
    encode(): number[] {
        let dist = length(sub(this.start,this.stop))
        let substeps = Math.ceil(dist * 40)
        let cmds = []
        for(let i = 0; i < substeps; i++){
            let xy = add(mul(this.start, 1 - (i/substeps)), mul(this.stop, (i/substeps)));
            cmds.push(new XYCommand(xy));
        }
        cmds.push(new XYCommand(this.stop))
        return cmds.flatMap(cmd=>cmd.encode());
    }
}

export class CircleCommand implements Command {
    constructor(public r: number, public center: Vec2) {}
    encode(): number[] {
        let substeps = Math.ceil(this.r * 2 * Math.PI * 20);
        let cmds = [];
        for(let i = 0; i <= substeps; i++){
            let theta = i / substeps * Math.PI * 2;
            let xy = add(this.center, rotate([this.r,0], theta));
            cmds.push(new XYCommand(xy))
        }
        return cmds.flatMap(cmd=>cmd.encode());
    }
}

export class StatusCommand implements Command {
    constructor(public status: number) {}
    encode(): number[] {
        return [CMD_TYPES.Status, this.status, 0, 0, 0];
    }
}

export class MultiCommmand implements Command {
    constructor(public cmds: Command[]) {}
    encode(): number[] {
        return this.cmds.flatMap(cmd=>cmd.encode())
    }
}

export class MusicCommand implements Command {
    constructor(public note: string, public duration_s: number){}
    encode(): number[] {
        let notes = {
            'a': 440,
            'b': 466.16,
            'c': 523.25,
            'd': 587.33,
            'e': 659.26,
            'f': 698.46,
            'g': 783.99,
        }
        if(!notes[this.note]) throw new Error("bad note " + this.note);
        let delay = Math.round(1000000/notes[this.note]);
        let steps = Math.round(this.duration_s * 1000000 / delay);
        return [...new SpeedCommand(delay).encode(),
                ...new ZeroCommand().encode(),
                ...new MoveCommand([steps,steps,steps,steps]).encode(),
                ]
    }
}