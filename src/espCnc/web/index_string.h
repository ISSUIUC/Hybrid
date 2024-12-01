#pragma once
static constexpr const char * index_string = R"rawstring(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP CNC</title>
    <style>
        body {
            display: grid;
            grid-template-rows: 1fr;
            grid-template-columns: 1fr 1fr 3fr;
            width: 100vw;
            height: 100vh;
            padding: 10px;
            overflow: hidden;
        }
        canvas {
            background-color: lightgray;
            width: 100%;
            height: 50%;
        }
    </style>
</head>
<body>
    <div id="cnc0">
        <textarea id="code_input0" style="height: 50vh;"></textarea>
        <input type="button" value="go" id="go_button0">
        <input type="button" value="ESTOP" id="estop0">
        <div id="status0">
            WAITING...
        </div>
    </div>
    <div id="cnc1">
        <textarea id="code_input1" style="height: 50vh;"></textarea>
        <input type="button" value="go" id="go_button1">
        <input type="button" value="ESTOP" id="estop1">
        <div id="status1">
            WAITING...
        </div>
    </div>
    <div id="draw">
        <canvas id="canvas"></canvas>
        <input type="button" value="go" id="canvas_go">
    </div>
    <script src="bundle.js"></script>
</body>
</html>)rawstring";
static constexpr const char * js_string = R"rawstring((() => {
  // src/projection.ts
  var r_pencil = 15 * 0.8;
  var pencil_center = [-17 * 0.8, 0];
  function length(xy) {
    return Math.sqrt(xy[0] ** 2 + xy[1] ** 2);
  }
  function mul(xy, scale) {
    return [xy[0] * scale, xy[1] * scale];
  }
  function add(a, b) {
    return [a[0] + b[0], a[1] + b[1]];
  }
  function sub(a, b) {
    return [a[0] - b[0], a[1] - b[1]];
  }
  function rotate(v, theta) {
    return [
      v[0] * Math.cos(theta) + v[1] * -Math.sin(theta),
      v[0] * Math.sin(theta) + v[1] * Math.cos(theta)
    ];
  }
  function angle(v) {
    return Math.atan2(v[1], v[0]);
  }
  function coord_to_angle(xy) {
    const r1 = r_pencil;
    const r2 = length(xy);
    const d = length(pencil_center);
    const l = (r1 ** 2 - r2 ** 2 + d ** 2) / (2 * d);
    if (r1 ** 2 - l ** 2 < 0) {
      throw new Error("can't reach");
    }
    const h = Math.sqrt(r1 ** 2 - l ** 2);
    const common = add(mul(pencil_center, -l / d), pencil_center);
    const diff = mul([pencil_center[1], -pencil_center[0]], h / d);
    let inter = add(common, diff);
    if (inter[1] < 0) {
      inter = sub(common, diff);
    }
    const pencil_to_inter = sub(inter, pencil_center);
    const theta_pencil = angle(pencil_to_inter);
    const theta_xy = angle(xy);
    const theta_tip = angle(add(pencil_center, rotate([r_pencil, 0], theta_pencil)));
    const theta_paper = theta_tip - theta_xy;
    if (theta_paper > 1 || theta_paper < -1) throw new Error("angle extra");
    return [theta_paper, theta_pencil];
  }

  // src/commands.ts
  var CMD_TYPES = {
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
    Timed: 15
  };
  function concat(buffs) {
    const len = buffs.reduce((a, b) => a + b.length, 0);
    const together = new Uint8Array(len);
    let head = 0;
    for (const buff of buffs) {
      together.set(buff, head);
      head += buff.length;
    }
    return together;
  }
  function u32_from_array(arr) {
    return new Uint8Array(new Uint32Array(arr).buffer);
  }
  var StopNowCommand = class {
    constructor() {
    }
    encode() {
      return u32_from_array([CMD_TYPES.StopNow, 0, 0, 0, 0]);
    }
  };
  var StartCommand = class {
    constructor() {
    }
    encode() {
      return u32_from_array([CMD_TYPES.StartAll, 0, 0, 0, 0]);
    }
  };
  var SetupCommand = class {
    constructor() {
    }
    encode() {
      return new MultiCommmand([
        new StartCommand(),
        new MicrostepsCommand([256, 256, 256, 256]),
        new StealthChopCommand([1, 1, 1, 1]),
        new CoolStepCommand([1, 1, 1, 1]),
        new EnableCommand([1, 1, 1, 1])
      ]).encode();
    }
  };
  var StopCommand = class {
    constructor() {
    }
    encode() {
      return u32_from_array([CMD_TYPES.StopAll, 0, 0, 0, 0]);
    }
  };
  var SpeedCommand = class {
    constructor(delay_us) {
      this.delay_us = delay_us;
    }
    encode() {
      return u32_from_array([CMD_TYPES.SetSpeed, this.delay_us, 0, 0, 0]);
    }
  };
  var MoveCommand = class {
    constructor(positions) {
      this.positions = positions;
    }
    encode() {
      return u32_from_array([CMD_TYPES.MoveTo, ...this.positions]);
    }
  };
  var EnableCommand = class {
    constructor(enables) {
      this.enables = enables;
    }
    encode() {
      return u32_from_array([CMD_TYPES.Enable, ...this.enables]);
    }
  };
  var WaitCommand = class {
    constructor(delay_us) {
      this.delay_us = delay_us;
    }
    encode() {
      return u32_from_array([CMD_TYPES.Wait, this.delay_us, 0, 0, 0]);
    }
  };
  var MicrostepsCommand = class {
    constructor(microsteps) {
      this.microsteps = microsteps;
    }
    encode() {
      return u32_from_array([CMD_TYPES.SetMicrosteps, ...this.microsteps]);
    }
  };
  var ZeroCommand = class {
    constructor() {
    }
    encode() {
      return u32_from_array([CMD_TYPES.ZeroPosition, 0, 0, 0, 0]);
    }
  };
  var StealthChopCommand = class {
    constructor(enables) {
      this.enables = enables;
    }
    encode() {
      return u32_from_array([CMD_TYPES.StealthChop, ...this.enables]);
    }
  };
  var CoolStepCommand = class {
    constructor(enables) {
      this.enables = enables;
    }
    encode() {
      return u32_from_array([CMD_TYPES.CoolStep, ...this.enables]);
    }
  };
  var AbsoluteAngleCommand = class {
    constructor(angles) {
      this.angles = angles;
    }
    encode() {
      const full_rotation = 200 * 256;
      return new MoveCommand([
        this.angles[0] / Math.PI / 2 * full_rotation,
        this.angles[1] / Math.PI / 2 * full_rotation,
        this.angles[2] / Math.PI / 2 * full_rotation,
        this.angles[3] / Math.PI / 2 * full_rotation
      ]).encode();
    }
  };
  var XYCommand = class {
    constructor(xy) {
      this.xy = xy;
    }
    encode() {
      let coords = coord_to_angle(this.xy);
      return new AbsoluteAngleCommand([...coords, 0, 0]).encode();
    }
  };
  var LineCommand = class {
    constructor(start, stop) {
      this.start = start;
      this.stop = stop;
    }
    encode() {
      let dist = length(sub(this.start, this.stop));
      let substeps = Math.ceil(dist * 40);
      let cmds = [];
      for (let i = 0; i < substeps; i++) {
        let xy = add(mul(this.start, 1 - i / substeps), mul(this.stop, i / substeps));
        cmds.push(new XYCommand(xy));
      }
      cmds.push(new XYCommand(this.stop));
      return concat(cmds.map((cmd) => cmd.encode()));
    }
  };
  var CircleCommand = class {
    constructor(r, center) {
      this.r = r;
      this.center = center;
    }
    encode() {
      let substeps = Math.ceil(this.r * 2 * Math.PI * 20);
      let cmds = [];
      for (let i = 0; i <= substeps; i++) {
        let theta = i / substeps * Math.PI * 2;
        let xy = add(this.center, rotate([this.r, 0], theta));
        cmds.push(new XYCommand(xy));
      }
      return concat(cmds.map((cmd) => cmd.encode()));
    }
  };
  var StatusCommand = class {
    constructor(status) {
      this.status = status;
    }
    encode() {
      return u32_from_array([CMD_TYPES.SetStatus, this.status, 0, 0, 0]);
    }
  };
  var MultiCommmand = class {
    constructor(cmds) {
      this.cmds = cmds;
    }
    encode() {
      return concat(this.cmds.map((cmd) => cmd.encode()));
    }
  };
  var TimedMoveCommand = class {
    constructor(step_sequence) {
      this.step_sequence = step_sequence;
    }
    encode_timings() {
      let last_cmd_index = 0;
      let timing_commands = [];
      for (let i = 0; i < this.step_sequence.length; i++) {
        if (this.step_sequence[i] != 0) {
          timing_commands.push(i - last_cmd_index | this.step_sequence[i] << 8);
          last_cmd_index = i;
        }
        if (i - last_cmd_index == 256) {
          timing_commands.push(i - last_cmd_index | 0);
          last_cmd_index = i;
        }
      }
      timing_commands.push(this.step_sequence.length - last_cmd_index);
      return new Uint16Array(timing_commands);
    }
    encode() {
      let commands = this.encode_timings();
      let header = [
        CMD_TYPES.NewTimingReference,
        0,
        0,
        0,
        0,
        CMD_TYPES.Timed,
        commands.length,
        0,
        0,
        0
      ];
      return concat([
        u32_from_array(header),
        new Uint8Array(commands.buffer)
      ]);
    }
  };

  // src/ui.ts
  function assert_len(arr, len) {
    if (arr.length != len) throw new Error(`Bad length (${arr.length} != ${len})`);
  }
  function parse_command(cmd) {
    if (cmd[0] == "speed") {
      assert_len(cmd, 2);
      return new SpeedCommand((0, eval)(cmd[1]));
    } else if (cmd[0] == "move") {
      assert_len(cmd, 5);
      let nums = cmd.slice(1).map(eval);
      return new MoveCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if (cmd[0] == "start") {
      assert_len(cmd, 1);
      return new StartCommand();
    } else if (cmd[0] == "stop") {
      assert_len(cmd, 1);
      return new StopCommand();
    } else if (cmd[0] == "enable") {
      assert_len(cmd, 5);
      let nums = cmd.slice(1).map(Number);
      return new EnableCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if (cmd[0] == "wait") {
      assert_len(cmd, 2);
      return new WaitCommand(Number(cmd[1]) * 1e6 | 0);
    } else if (cmd[0] == "micro" || cmd[0] == "microsteps") {
      assert_len(cmd, 5);
      let nums = cmd.slice(1).map(Number);
      return new MicrostepsCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if (cmd[0] == "zero") {
      assert_len(cmd, 1);
      return new ZeroCommand();
    } else if (cmd[0] == "stealth") {
      assert_len(cmd, 5);
      let nums = cmd.slice(1).map(Number);
      return new StealthChopCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if (cmd[0] == "cool") {
      assert_len(cmd, 5);
      let nums = cmd.slice(1).map(Number);
      return new CoolStepCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if (cmd[0] == "xy") {
      assert_len(cmd, 3);
      let nums = cmd.slice(1).map(Number);
      return new XYCommand([nums[0], nums[1]]);
    } else if (cmd[0] == "line") {
      assert_len(cmd, 5);
      let nums = cmd.slice(1).map(Number);
      return new LineCommand([nums[0], nums[1]], [nums[2], nums[3]]);
    } else if (cmd[0] == "circle") {
      assert_len(cmd, 4);
      let nums = cmd.slice(1).map(Number);
      return new CircleCommand(nums[0], [nums[1], nums[2]]);
    } else if (cmd[0] == "status") {
      assert_len(cmd, 2);
      return new StatusCommand(Number(cmd[1]));
    } else if (cmd[0] == "setup") {
      assert_len(cmd, 1);
      return new SetupCommand();
    } else if (cmd[0] == "test") {
      return test_timing();
    } else {
      throw new Error("unknown " + cmd.toString());
    }
  }
  function test_timing() {
    let steps = new Uint8Array(1e6);
    let h = 0;
    for (let i = 0; i < 1e3; i++) {
      steps[h] = 4;
      h += 100;
    }
    console.log(h);
    return new TimedMoveCommand(steps);
  }
  function apply_macro(macro, stack) {
    if (macro[0] == "#repeat") {
      assert_len(macro, 2);
      let last = stack.pop();
      let ct = Number(macro[1]);
      stack.push(new MultiCommmand(new Array(ct).fill(last)));
    }
  }
  var UI = class {
    ws;
    go_button;
    estop;
    code;
    status;
    waitin_promises = [];
    constructor(ws_url, go_id, estop_id, code_id, status_id) {
      this.ws = new WebSocket(ws_url);
      this.ws.onmessage = (ev) => {
        let s = window.atob(ev.data);
        let d = new Uint8Array(s.length);
        for (let i = 0; i < s.length; i++) {
          let v = s.charCodeAt(i);
          d[i] = v;
        }
        let u32 = new Uint32Array(d.buffer);
        let status_index = u32[0];
        this.#new_status(status_index);
      };
      this.code = document.getElementById(code_id);
      this.go_button = document.getElementById(go_id);
      this.estop = document.getElementById(estop_id);
      this.status = document.getElementById(status_id);
      this.ws.onopen = () => {
        console.log("WS opened");
        this.status.innerHTML = "CONNECTED";
      };
      this.ws.onclose = () => {
        console.log("WS closed");
        this.status.innerHTML = "DISCONNECTED";
      };
      this.ws.onerror = (e) => {
        console.log(e);
        this.status.innerHTML = "ERROR";
      };
      this.estop.addEventListener("click", () => {
        this.ws.send(new Uint32Array(new StopNowCommand().encode()));
      });
      this.go_button.addEventListener("click", () => {
        this.upload_code();
      });
    }
    execute(cmds) {
      const binary = concat(cmds.flatMap((x) => x.encode()));
      console.log(binary);
      this.ws.send(binary);
    }
    #new_status(status_index) {
      this.status.innerHTML = status_index + "";
      this.waitin_promises = this.waitin_promises.filter((x) => {
        let { n, res } = x;
        if (n == status_index) {
          res();
          return false;
        } else {
          return true;
        }
      });
    }
    upload_code() {
      const code = this.code.value;
      const lines2 = code.split("\n");
      const cmds = lines2.map((l) => l.toLowerCase()).map((l) => l.split(/\s/).filter((x) => x)).filter((x) => x.length);
      let commands = [];
      for (const cmd of cmds) {
        if (cmd[0].startsWith("#")) {
          apply_macro(cmd, commands);
        } else {
          commands.push(parse_command(cmd));
        }
      }
      this.execute(commands);
    }
    async wait_for_status(status) {
      return new Promise((res, rej) => {
        this.waitin_promises.push({ n: status, res });
      });
    }
  };

  // src/main.ts
  var ui0 = new UI("ws://192.168.234.227/ws", "go_button0", "estop0", "code_input0", "status0");
  var ui1 = new UI("ws://192.168.234.216/ws", "go_button1", "estop1", "code_input1", "status1");
  var CANVAS_GO = document.getElementById("canvas_go");
  var canvas = document.getElementById("canvas");
  canvas.width = canvas.clientWidth;
  canvas.height = canvas.clientHeight;
  var ctx = canvas.getContext("2d");
  var mouse = null;
  function map_coords(xy) {
    return [-(xy[0] / 100), xy[1] / 100 + 6];
  }
  var lines = [];
  canvas.addEventListener("mousedown", (ev) => {
    const xy = [ev.clientX - canvas.offsetLeft, ev.clientY - canvas.offsetTop];
    mouse = xy;
  });
  canvas.addEventListener("mousemove", (ev) => {
    if (!mouse) return;
    const xy = [ev.clientX - canvas.offsetLeft, ev.clientY - canvas.offsetTop];
    let next_mouse = xy;
    ctx.moveTo(mouse[0], mouse[1]);
    ctx.lineTo(next_mouse[0], next_mouse[1]);
    ctx.stroke();
    let m1 = map_coords(mouse);
    let m2 = map_coords(next_mouse);
    mouse = next_mouse;
    lines.push([m1, m2]);
  });
  canvas.addEventListener("mouseup", (ev) => {
    mouse = null;
  });
  window.addEventListener("keypress", (ev) => {
    if (ev.key == " ") {
      mouse = null;
    }
  });
  function pt_eq(a, b) {
    return a[0] == b[0] && a[1] == b[1];
  }
  CANVAS_GO.addEventListener("click", (ev) => {
    let all_steps = [];
    let step = [];
    let current_pos = [-1, -1];
    for (const line of lines) {
      if (pt_eq(line[0], current_pos)) {
        step.push(line);
      } else {
        all_steps.push(step);
        step = [line];
      }
      current_pos = line[1];
    }
    all_steps.push(step);
    all_steps = all_steps.filter((x) => x.length != 0);
    execute_lines(all_steps);
  });
  async function pencil_up() {
    ui1.execute([new MoveCommand([1300, 0, 0, 0]), new StatusCommand(101)]);
    await ui1.wait_for_status(101);
  }
  async function pencil_down() {
    ui1.execute([new MoveCommand([1150, 0, 0, 0]), new StatusCommand(102)]);
    await ui1.wait_for_status(102);
  }
  var global_counter = 1e3;
  async function draw_lines(lines2) {
    let n = global_counter++;
    let cmds = [];
    for (const line of lines2) {
      cmds.push(new LineCommand(line[0], line[1]));
    }
    ui0.execute([...cmds, new StatusCommand(n)]);
    await ui0.wait_for_status(n);
  }
  async function move_to(xy) {
    let n = global_counter++;
    ui0.execute([new XYCommand(xy), new StatusCommand(n)]);
    await ui0.wait_for_status(n);
  }
  async function execute_lines(steps) {
    for (const step of steps) {
      await pencil_up();
      await move_to(step[0][0]);
      await pencil_down();
      await draw_lines(step);
    }
    await pencil_up();
  }
})();
//# sourceMappingURL=bundle.js.map
)rawstring";
