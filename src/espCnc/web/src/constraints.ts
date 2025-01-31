import { Object3D } from "three"

export type ForwardConstraint = {
    target: Object3D,
    evaluate: (positions: number[], obj: Object3D)=>void;
}