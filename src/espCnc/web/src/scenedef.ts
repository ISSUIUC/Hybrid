import { Object3D, Vector3 } from "three";
import { ForwardConstraint } from "./constraints";

type ConstraintDef = {
    target: string,
    evaluate: (positions: number[], obj: Object3D)=>void;
}

function lookup_name(name: string, scene: Object3D): Object3D | null {
    return lookup_name_recur(name.split('.'), scene);
}

function lookup_name_recur(name_parts: string[], obj: Object3D): Object3D | null{
    if(name_parts.length == 0) return obj;
    for(const child of obj.children) {
        if(child.name == name_parts[0]) {
            return lookup_name_recur(name_parts.slice(1), child)
        }
    }
    return null;
}

function rotateConstraint(src_idx: number, factor: [number,number,number], offset?: [number,number,number]) {
    let o = offset || [0,0,0]
    return (pos: number[], obj: Object3D)=>{
        obj.rotation.x = o[0] + factor[0] * (pos[src_idx] % (256 * 200)) / (256 * 200) * 2 * Math.PI;
        obj.rotation.y = o[1] + factor[1] * (pos[src_idx] % (256 * 200)) / (256 * 200) * 2 * Math.PI;
        obj.rotation.z = o[2] + factor[2] * (pos[src_idx] % (256 * 200)) / (256 * 200) * 2 * Math.PI;
    }
}

function linearConstraint(src_idx: number, factor: [number,number,number], offset: [number,number,number]) {
    return (pos: number[], obj: Object3D)=>{
        obj.position.x = offset[0] + factor[0] * (pos[src_idx] / (256*200))
        obj.position.y = offset[1] + factor[1] * (pos[src_idx] / (256*200))
        obj.position.z = offset[2] + factor[2] * (pos[src_idx] / (256*200))
    }
}

const scene_definition: ConstraintDef[] = [
    {
        target: "Stepper.Shaft",
        evaluate: rotateConstraint(0, [0,1,0])
    },
    {
        target: "Gantry",
        evaluate: linearConstraint(0, [0,0,4/100], [0,22/1000,0])
    },
    {
        target: "Idler",
        evaluate: rotateConstraint(0, [1,0,0], [0,0,Math.PI/2])
    },
    {
        target: "Stepper001.Shaft001",
        evaluate: rotateConstraint(1, [0,1,0])
    },
    {
        target: "Stepper002.Shaft002",
        evaluate: rotateConstraint(2, [0,1,0])
    }
]

export function make_constraints(scene: Object3D): ForwardConstraint[] {
    return scene_definition.map(def=>{
        let obj = lookup_name(def.target, scene);
        if(!obj) throw new Error("Object not found: " + def.target);
        return {target: obj, evaluate: def.evaluate};
    });
}