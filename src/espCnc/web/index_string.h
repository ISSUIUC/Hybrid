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
      console.log(this.sequence);
      let buff = new Uint8Array(this.sequence.byteLength + 4);
      let u32 = new Uint32Array(buff.buffer);
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
      let parts = [new NewTimingReferenceCommand()];
      let chunk_ct = Math.ceil(commands.length / MAX_CHUNK);
      for (let i = 0; i < chunk_ct; i++) {
        let chunk = commands.subarray(i * MAX_CHUNK, (i + 1) * MAX_CHUNK);
        parts.push(new ContinuousTimedSequenceCommand(chunk));
      }
      console.log(parts);
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
  var ExecutionSession = class {
    constructor(ws) {
      this.ws = ws;
    }
    cmd_index = 0;
    build_execution(commands, base_index) {
      let chunks = [];
      let index = base_index;
      for (const cmd of commands) {
        let micros = cmd.encode();
        for (const micro of micros) {
          let chunk = new Uint8Array(4 + (micro.data?.length | 0));
          let u16 = new Uint16Array(chunk.buffer);
          u16[0] = micro.command_type;
          u16[1] = index;
          if (micro.data) {
            chunk.set(micro.data, 4);
          }
          chunks.push(chunk);
        }
        index++;
      }
      return concat(chunks);
    }
    execute(commands) {
      const binary = this.build_execution(commands, this.cmd_index);
      this.cmd_index += commands.length;
      console.log(binary);
      this.ws.send(binary);
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
    } else {
      throw new Error("unknown " + cmd.toString());
    }
  }
  function test_timing() {
    let ct = 1e6;
    let steps = new Uint8Array(ct);
    for (let i = 0; i < ct; i++) {
      let t1 = i / ct * 2 * Math.PI;
      let t0 = (i - 1) / ct * 2 * Math.PI;
      let s1 = Math.floor(Math.cos(t1) * 256 * 50);
      let s2 = Math.floor(Math.cos(t0) * 256 * 50);
      if (s1 > s2) {
        steps[i] = 4;
      }
      if (s1 < s2) {
        steps[i] = 68;
      }
    }
    return new TimedMoveCommand(steps);
  }
  var UI = class {
    ws;
    go_button;
    estop;
    code;
    status;
    waiting_promises = [];
    session;
    constructor(ws_url, go_id, estop_id, code_id, status_id) {
      this.ws = new WebSocket(ws_url);
      this.session = new ExecutionSession(this.ws);
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
        let cmds = [new EStopCommand()];
        this.session.execute(cmds);
      });
      this.go_button.addEventListener("click", () => {
        this.upload_code();
      });
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
    upload_code() {
      const code = this.code.value;
      const lines = code.split("\n");
      const cmds = lines.map((l) => l.toLowerCase()).map((l) => l.split(/\s/).filter((x) => x)).filter((x) => x.length);
      let commands = [];
      for (const cmd of cmds) {
        commands.push(parse_command(cmd));
      }
      this.session.execute(commands);
    }
    async wait_for_status(status) {
      return new Promise((res, rej) => {
        this.waiting_promises.push({ n: status, res });
      });
    }
  };

  // src/main.ts
  var ui0 = new UI("ws://192.168.234.227/ws", "go_button0", "estop0", "code_input0", "status0");
  var ui1 = new UI("ws://192.168.4.117/ws", "go_button1", "estop1", "code_input1", "status1");
  var CANVAS_GO = document.getElementById("canvas_go");
  var canvas = document.getElementById("canvas");
  canvas.width = canvas.clientWidth;
  canvas.height = canvas.clientHeight;
})();
//# sourceMappingURL=bundle.js.map
)rawstring";
