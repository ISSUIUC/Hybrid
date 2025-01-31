import * as THREE from "three"
import { GLTF, GLTFLoader } from "three/addons/loaders/GLTFLoader.js";
import { RGBELoader } from "three/addons/loaders/RGBELoader.js"
import { CMD_TYPES, Command, MicroCommand, TimedMoveCommand } from "./session";
import { ForwardConstraint } from "./constraints";
import { make_constraints } from "./scenedef";

function concat_i32(buffs: Int32Array[]) {
    const len = buffs.reduce((a,b)=>a + b.length, 0);
    const together = new Int32Array(len)
    let head = 0;
    for(const buff of buffs) {
        together.set(buff, head);
        head += buff.length;
    }
    return together;
}

export class Preview {
    scene: THREE.Scene;
    camera: THREE.Camera;
    renderer: THREE.WebGLRenderer;
    pressed: Map<String, boolean> = new Map();
    gltf_loader: GLTFLoader = new GLTFLoader();
    texture_loader: RGBELoader = new RGBELoader();
    mouse_down = false;
    camera_rot = [0,0];
    positions: Int32Array[];
    stepper_count = 4;
    constraints: ForwardConstraint[];

    constructor(canvas: HTMLCanvasElement) {
        this.scene = new THREE.Scene();
        this.camera = new THREE.PerspectiveCamera(90, canvas.clientWidth / canvas.clientHeight, 0.0001, 1);
        this.renderer = new THREE.WebGLRenderer({canvas});
        this.camera.position.z = 50/1000;
        this.camera.position.y = 50/1000;
        this.positions = new Array(this.stepper_count).fill(new Int32Array([0]));
        window.addEventListener("keydown", key=>{
            this.pressed.set(key.key, true);
        })
        window.addEventListener("keyup", key=>{
            this.pressed.set(key.key, false);
        })
        canvas.addEventListener("mousedown", ev=>{
            this.mouse_down = true;
        })
        window.addEventListener("mouseup", ev=>{
            this.mouse_down = false;
        })
        canvas.addEventListener("mousemove", ev=>{
            if(this.mouse_down) {
                this.camera_rot[0] -= ev.movementX / 100;
                this.camera_rot[1] -= ev.movementY / 100;
                this.camera.rotation.x = 0;
                this.camera.rotation.y = 0;
                this.camera.rotation.z = 0;
                this.camera.rotateY(this.camera_rot[0]);
                this.camera.rotateX(this.camera_rot[1]);
            }
        })
    }

    async load_assets() {
        let gltf = await this.load_gltf("stepper.glb");
        this.scene.add(gltf.scene)
        this.constraints = make_constraints(gltf.scene);
        let hdr = await this.load_envmap("rostock_laage_airport_4k.hdr");
        this.set_environment(hdr);
    }

    async load_gltf(path: string): Promise<GLTF> {
        return this.gltf_loader.loadAsync(path);
    }

    async load_hdr(path: string): Promise<THREE.DataTexture> {
        return this.texture_loader.loadAsync(path);
    }

    async load_envmap(path: string): Promise<THREE.Texture> {
        let tex = await this.load_hdr(path);
        let gen = new THREE.PMREMGenerator(this.renderer);
        let env = gen.fromEquirectangular(tex).texture;

        return env;
    }

    set_program(program: MicroCommand[]) {
        let current_positions = new Array(this.stepper_count).fill(0);
        let time = 0;
        let movement_blocks = [];

        for(const cmd of program) {
            if(cmd.command_type == CMD_TYPES.Timed) {
                let u32 = new Uint32Array(cmd.data.buffer, cmd.data.byteOffset, cmd.data.byteLength / 4);
                let u16 = new Uint16Array(cmd.data.buffer, cmd.data.byteOffset + 4, (cmd.data.byteLength - 4) / 2);
                let ct = u32[0];
                let s = this.decode(u16, ct, time, current_positions);
                current_positions = s.end_positions;
                time = s.end_time;
                movement_blocks.push(s.positions);
            }
        }

        for(let s = 0; s < this.stepper_count; s++){
            this.positions[s] = concat_i32(movement_blocks.map(m=>m[s]));
        }
    }

    set_time(time_s: number) {
        let time_us = (time_s * 1e6) | 0;
        time_us = Math.max(Math.min(time_us,this.positions[0].length-1),0);
        let pos = this.positions.map(p=>p[time_us]);
        this.constraints.forEach(c=>c.evaluate(pos,c.target));
    }

    private decode(cmds: Uint16Array, count: number, t0: number, p0: number[]): {positions: Int32Array[], end_time: number, end_positions: number[]} {
        let total_time = 0;

        for(let i = 0; i < count; i++) {
            total_time += cmds[i] & 0xff;
        }

        let current_positons = p0.slice();
        let time_positions = new Array(this.stepper_count).fill(0).map(()=>new Int32Array(total_time));
        let t = 0;
        for(let i = 0; i < count; i++) {
            let cmd = cmds[i];
            let delay = cmd & 0xff;
            let step = (cmd >> 8) & 0xff
            for(let d = t; d < t + delay; d++){
                for(let channel = 0; channel < this.stepper_count; channel++) {
                    time_positions[channel][d] = current_positons[channel]
                }
            }
            t += delay;
            for(let channel = 0; channel < this.stepper_count; channel++) {
                if(step & (0x1 << channel)) {
                    if(step & (0x10 << channel)) {
                        current_positons[channel]--;
                    } else {
                        current_positons[channel]++;
                    }
                }
            }
        }

        return {positions: time_positions, end_time: t0 + total_time, end_positions: current_positons};
    }

    set_environment(env: THREE.Texture) {
        this.scene.background = env;
        this.scene.backgroundBlurriness = 0.8;
        this.scene.environment = env;
        this.scene.environmentIntensity = 1;
    }

    add_object(obj: THREE.Object3D) {
        this.scene.add(obj)
    }

    update_camera(dt: number) {
        let dp = new THREE.Vector3();
        if(this.pressed.get("a")) {
            dp.x -= 1;
        }
        if(this.pressed.get("d")) {
            dp.x += 1;
        }
        if(this.pressed.get("w")) {
            dp.z -= 1;
        }
        if(this.pressed.get("s")) {
            dp.z += 1;
        }
        if(this.pressed.get("q")) {
            dp.y -= 1;
        }
        if(this.pressed.get("e")) {
            dp.y += 1;
        }
        dp.multiplyScalar(dt / 8);
        let rot = new THREE.Matrix4();
        rot.makeRotationY(this.camera_rot[0]);
        dp.applyMatrix4(rot);
        let p = this.camera.position.add(dp)
        this.camera.position.x = p.x;
        this.camera.position.y = p.y;
        this.camera.position.z = p.z;
    }

    draw() {
        this.renderer.render(this.scene, this.camera)
    }
}