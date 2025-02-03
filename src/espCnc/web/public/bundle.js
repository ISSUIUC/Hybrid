(() => {
  // src/session.ts
  var CMD_TYPES = {
    StopNow: 0,
    StopAll: 1,
    StartAll: 2,
    Enable: 3,
    SetMicrosteps: 4,
    StealthChop: 11,
    CoolStep: 12,
    SetStatus: 13,
    NewTimingReference: 14,
    Timed: 15
  };
  var MAX_CHUNK = 4096;
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
  var MicroCommand = class {
    constructor(command_type, data) {
      this.command_type = command_type;
      this.data = data;
    }
    index = 0;
  };
  var EStopCommand = class {
    encode() {
      return [new MicroCommand(CMD_TYPES.StopNow)];
    }
  };
  var StopCommand = class {
    encode() {
      return [new MicroCommand(CMD_TYPES.StopAll)];
    }
  };
  var StartCommand = class {
    encode() {
      return [new MicroCommand(CMD_TYPES.StartAll)];
    }
  };
  var MicrostepsCommand = class {
    constructor(microsteps) {
      this.microsteps = microsteps;
    }
    encode() {
      return [new MicroCommand(CMD_TYPES.SetMicrosteps, new Uint8Array(new Uint16Array(this.microsteps).buffer))];
    }
  };
  var StealthChopCommand = class {
    constructor(enable) {
      this.enable = enable;
    }
    encode() {
      return [new MicroCommand(CMD_TYPES.StealthChop, new Uint8Array(this.enable))];
    }
  };
  var CoolStepCommand = class {
    constructor(enable) {
      this.enable = enable;
    }
    encode() {
      return [new MicroCommand(CMD_TYPES.CoolStep, new Uint8Array(this.enable))];
    }
  };
  var EnableCommand = class {
    constructor(enable) {
      this.enable = enable;
    }
    encode() {
      return [new MicroCommand(CMD_TYPES.Enable, new Uint8Array(this.enable))];
    }
  };
  var NewTimingReferenceCommand = class {
    constructor() {
    }
    encode() {
      return [new MicroCommand(CMD_TYPES.NewTimingReference)];
    }
  };
  var ContinuousTimedSequenceCommand = class {
    constructor(sequence) {
      this.sequence = sequence;
      if (sequence.length > MAX_CHUNK) throw new Error("Too big chunk");
    }
    encode() {
      let buff = new Uint8Array(this.sequence.byteLength + 4);
      let u32 = new Uint32Array(buff.buffer, 0, 1);
      u32[0] = this.sequence.length;
      buff.set(new Uint8Array(this.sequence.buffer, this.sequence.byteOffset, this.sequence.byteLength), 4);
      return [new MicroCommand(CMD_TYPES.Timed, buff)];
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
        if (i - last_cmd_index == 255) {
          timing_commands.push(i - last_cmd_index | 0);
          last_cmd_index = i;
        }
      }
      timing_commands.push(this.step_sequence.length - last_cmd_index);
      return new Uint16Array(timing_commands);
    }
    encode() {
      let commands = this.encode_timings();
      let parts = [new NewTimingReferenceCommand()];
      let chunk_ct = Math.ceil(commands.length / MAX_CHUNK);
      for (let i = 0; i < chunk_ct; i++) {
        let chunk = commands.subarray(i * MAX_CHUNK, (i + 1) * MAX_CHUNK);
        parts.push(new ContinuousTimedSequenceCommand(chunk));
      }
      return new MultiCommmand(parts).encode();
    }
  };
  var MultiCommmand = class {
    constructor(commands) {
      this.commands = commands;
    }
    encode() {
      return this.commands.flatMap((cmd) => cmd.encode());
    }
  };
  var SetupCommand = class {
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
  var LineCommand = class {
    constructor(dst) {
      this.dst = dst;
    }
    encode() {
      const mm_per_step = 40 / (256 * 200);
      const step_per_mm = 256 * 200 / 40;
      const max_speed = 200;
      const max_accel = 200;
      let major_axis = Math.max(Math.abs(this.dst[0]), Math.abs(this.dst[1]));
      let accel_time = max_speed / max_accel;
      let accel_dist = accel_time ** 2 * max_accel / 2;
      let pos = 0;
      let dt = 1e-6;
      let steps = [];
      let i = 0;
      let dist = 0;
      let calc_step = (pos2, dpos) => {
        let x_forward = 4;
        let x_backward = 68;
        let y_forward = 8;
        let y_backward = 136;
        let pos_steps = pos2 * step_per_mm;
        let next_pos_steps = (pos2 + dpos) * step_per_mm;
        let step = 0;
        if (Math.round(pos_steps * this.dst[0] / major_axis) < Math.round(next_pos_steps * this.dst[0] / major_axis)) {
          step |= x_forward;
          dist += 1;
        }
        if (Math.round(pos_steps * this.dst[0] / major_axis) > Math.round(next_pos_steps * this.dst[0] / major_axis)) {
          step |= x_backward;
          dist -= 1;
        }
        if (Math.round(pos_steps * this.dst[1] / major_axis) < Math.round(next_pos_steps * this.dst[1] / major_axis)) {
          step |= y_forward;
        }
        if (Math.round(pos_steps * this.dst[1] / major_axis) > Math.round(next_pos_steps * this.dst[1] / major_axis)) {
          step |= y_backward;
        }
        return step;
      };
      for (; pos < major_axis / 2 && pos < accel_dist; i++) {
        let vel = i * dt * max_accel;
        steps.push(calc_step(pos, vel * dt));
        pos += vel * dt;
      }
      while (pos < major_axis - accel_dist) {
        let vel = i * dt * max_accel;
        steps.push(calc_step(pos, vel * dt));
        pos += vel * dt;
      }
      for (; pos < major_axis; i--) {
        let vel = i * dt * max_accel;
        steps.push(calc_step(pos, vel * dt));
        pos += vel * dt;
      }
      return new TimedMoveCommand(new Uint8Array(steps)).encode();
    }
  };
  var ExecutionSession = class {
    constructor() {
    }
    compile_commands(commands, base_index) {
      let index = base_index;
      let micro_commands = [];
      for (const cmd of commands) {
        let micros = cmd.encode();
        for (const micro of micros) {
          micro.index = index;
          micro_commands.push(micro);
        }
        index++;
      }
      return micro_commands;
    }
    serialize_micros(micros) {
      let chunks = [];
      for (const micro of micros) {
        let chunk = new Uint8Array(4 + (micro.data?.length | 0));
        let u16 = new Uint16Array(chunk.buffer);
        u16[0] = micro.command_type;
        u16[1] = micro.index;
        if (micro.data) {
          chunk.set(micro.data, 4);
        }
        chunks.push(chunk);
      }
      return concat(chunks);
    }
  };

  // src/ui.ts
  function assert_len(arr, len) {
    if (arr.length != len) throw new Error(`Bad length (${arr.length} != ${len})`);
  }
  function parse_command(cmd) {
    if (cmd[0] == "start") {
      assert_len(cmd, 1);
      return new StartCommand();
    } else if (cmd[0] == "stop") {
      assert_len(cmd, 1);
      return new StopCommand();
    } else if (cmd[0] == "enable") {
      assert_len(cmd, 5);
      let nums = cmd.slice(1).map(Number);
      return new EnableCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if (cmd[0] == "micro" || cmd[0] == "microsteps") {
      assert_len(cmd, 5);
      let nums = cmd.slice(1).map(Number);
      return new MicrostepsCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if (cmd[0] == "stealth") {
      assert_len(cmd, 5);
      let nums = cmd.slice(1).map(Number);
      return new StealthChopCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if (cmd[0] == "cool") {
      assert_len(cmd, 5);
      let nums = cmd.slice(1).map(Number);
      return new CoolStepCommand([nums[0], nums[1], nums[2], nums[3]]);
    } else if (cmd[0] == "setup") {
      assert_len(cmd, 1);
      return new SetupCommand();
    } else if (cmd[0] == "test") {
      return test_timing();
    } else if (cmd[0] == "line") {
      assert_len(cmd, 3);
      let nums = cmd.slice(1).map(Number);
      return new LineCommand([nums[0], nums[1]]);
    } else {
      throw new Error("unknown " + cmd.toString());
    }
  }
  function test_timing() {
    let cmds = [];
    let x = 0;
    let y = 0;
    for (let i = 0; i < 40; i++) {
      let t = i / 4 * 2 * Math.PI;
      let ex = Math.sin(t) * 30;
      let ey = Math.cos(t) * 30;
      cmds.push(new LineCommand([ex - x, ey - y]));
      x = ex;
      y = ey;
    }
    cmds.push(new LineCommand([-x, -y]));
    return new MultiCommmand(cmds);
  }
  var UI = class {
    ws;
    go_button;
    estop;
    code;
    status;
    onMicros = null;
    waiting_promises = [];
    session;
    constructor(ws_url, go_id, estop_id, code_id, status_id) {
      this.session = new ExecutionSession();
      this.code = document.getElementById(code_id);
      this.go_button = document.getElementById(go_id);
      this.estop = document.getElementById(estop_id);
      this.status = document.getElementById(status_id);
      this.estop.addEventListener("click", () => {
        let cmds = [new EStopCommand()];
        this.execute(cmds);
      });
      this.go_button.addEventListener("click", () => {
        this.upload_code();
      });
      if (ws_url) {
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
          this.new_status(status_index);
        };
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
      }
    }
    new_status(status_index) {
      this.status.innerHTML = status_index + "";
      this.waiting_promises = this.waiting_promises.filter((x) => {
        let { n, res } = x;
        if (n == status_index) {
          res();
          return false;
        } else {
          return true;
        }
      });
    }
    execute(cmds) {
      let micros = this.session.compile_commands(cmds, 0);
      let binary = this.session.serialize_micros(micros);
      if (this.onMicros) this.onMicros(micros);
      if (this.ws) this.ws.send(binary);
    }
    upload_code() {
      const code = this.code.value;
      const lines = code.split("\n");
      const cmds = lines.map((l) => l.toLowerCase()).map((l) => l.split(/\s/).filter((x) => x)).filter((x) => x.length);
      let commands = [];
      for (const cmd of cmds) {
        commands.push(parse_command(cmd));
      }
      this.execute(commands);
    }
    async wait_for_status(status) {
      return new Promise((res, rej) => {
        this.waiting_promises.push({ n: status, res });
      });
    }
  };

  // src/main.ts
  var ui = new UI("ws://192.168.4.124/ws", "go_button0", "estop0", "code_input0", "status0");
})();
//# sourceMappingURL=bundle.js.map
