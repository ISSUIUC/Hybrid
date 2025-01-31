// import { Euler, Matrix3, Vector3 } from "three";

// export class PhysicsState {
//     constructor(public readonly position: Vector3, public readonly rotation: Vector3){}
// }

// export interface Constraint {
//     forward(knowledge: PhysicsState[]): PhysicsState;
//     reverse(knowledge: PhysicsState[], target: PhysicsState);
// }

// export class ConstantConstraint implements Constraint{
//     constructor(private state: PhysicsState){}
//     forward(knowledge: PhysicsState[]): PhysicsState {
//         return this.state
//     }
//     reverse(knowledge: PhysicsState[], target: PhysicsState) {
//         throw new Error("Method not implemented.");
//     }
// }

// export class CopyConstraint implements Constraint{
//     constructor(private src_idx: number) {}
//     forward(knowledge: PhysicsState[]) {
//         return knowledge[this.src_idx];
//     }
//     reverse(knowledge: PhysicsState[], target: PhysicsState): Map<number, PhysicsState> {
//         return new Map([[this.src_idx, target]]);
//     }
// }

// export class LinearConstraint implements Constraint{
//     constructor(private src_idx: number, private jacobian: Matrix3) {}
//     forward(knowledge: PhysicsState[]): PhysicsState {
//         let pos = knowledge[this.src_idx].rotation.applyMatrix3(this.jacobian);
//         return new PhysicsState(pos, new Vector3());
//     }
//     reverse(knowledge: PhysicsState[], target: PhysicsState) {
//         throw new Error("Method not implemented.");
//     }
// }

// export class InputConstraint implements Constraint{
//     constructor(private input_idx: number){}
//     forward(knowledge: PhysicsState[]): PhysicsState {
//         return new PhysicsState(new Vector3(), new Vector3());
//     }
//     reverse(knowledge: PhysicsState[], target: PhysicsState) {
//         throw new Error("Method not implemented.");
//     }
// }

