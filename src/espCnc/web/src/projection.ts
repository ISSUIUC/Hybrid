const r_pencil = 15*0.8;
const pencil_center: Vec2 = [-17*0.8, 0];

export type Vec2 = [number,number]
export function angle_to_coord(paper_pencil: Vec2): Vec2 {
    const pencil_world = add(pencil_center, rotate([r_pencil,0], paper_pencil[1]));
    const paper = rotate(pencil_world, paper_pencil[0]);
    return paper;
}

export function length(xy: Vec2): number {
    return Math.sqrt(xy[0]**2 + xy[1]**2);
}

export function mul(xy: Vec2, scale: number): Vec2 {
    return [xy[0]*scale,xy[1]*scale];
}

export function dot(a: Vec2, b: Vec2): number {
    return a[0]*b[0] + a[1]*b[1];
}

export function normalize(xy: Vec2): Vec2 {
    return mul(xy, 1/length(xy));
}

export function add(a: Vec2, b: Vec2): Vec2 {
    return [a[0]+b[0],a[1]+b[1]];
}

export function sub(a: Vec2, b: Vec2): Vec2 {
    return [a[0]-b[0],a[1]-b[1]];
}


export function rotate(v: Vec2, theta: number): Vec2 {
    return [
        v[0] * Math.cos(theta) + v[1] * -Math.sin(theta),
        v[0] * Math.sin(theta) + v[1] * Math.cos(theta),
    ]
}

function angle(v: Vec2): number {
    return Math.atan2(v[1], v[0]);
}

export function coord_to_angle(xy: [number, number]): Vec2 {
    const r1 = r_pencil;
    const r2 = length(xy);
    const d = length(pencil_center);
    const l = (r1**2-r2**2+d**2)/(2*d);
    if(r1**2-l**2 < 0){
        throw new Error("can't reach");
    }
    const h = Math.sqrt(r1**2-l**2);

    const common = add(mul(pencil_center, -l/d), pencil_center);
    const diff = mul([pencil_center[1],-pencil_center[0]],h/d);
    let inter = add(common, diff);
    if(inter[1] < 0){
        inter = sub(common, diff);
    }

    const pencil_to_inter = sub(inter, pencil_center);
    const theta_pencil = angle(pencil_to_inter);
    const theta_xy = angle(xy);
    const theta_tip = angle(add(pencil_center, rotate([r_pencil,0],theta_pencil)))
    const theta_paper = theta_tip - theta_xy;


    if(theta_paper > 1 || theta_paper < -1) throw new Error("angle extra")
    return [theta_paper, theta_pencil]
}
