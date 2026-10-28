// ASVIO Model3D viewer : WebGL2 , no libraries. Reads .glb / .gltf ( embedded buffers ) / .ply ( ascii or binary ,
// gaussian splat plys coloured from their f_dc coefficients ) / .obj / .stl / .splat. Meshes are lit , point clouds
// are drawn as points. Drag rotates , wheel zooms , shift-drag or right-drag pans , double click resets.
"use strict";
window.ASVIOModel3D = (function () {
  // ------------------------------------------------------------------ parsers -> { positions , colors? , normals? , indices? }
  const SH_C0 = 0.28209479177387814;
  const COMPONENT = { 5120: Int8Array, 5121: Uint8Array, 5122: Int16Array, 5123: Uint16Array, 5125: Uint32Array, 5126: Float32Array };
  const SIZE = { SCALAR: 1, VEC2: 2, VEC3: 3, VEC4: 4, MAT4: 16 };

  function mat4Multiply(a, b) {
    const out = new Float32Array(16);
    for (let c = 0; c < 4; c++) for (let r = 0; r < 4; r++) {
      let s = 0;
      for (let k = 0; k < 4; k++) s += a[k * 4 + r] * b[c * 4 + k];
      out[c * 4 + r] = s;
    }
    return out;
  }
  function nodeMatrix(node) {
    if (node.matrix) return Float32Array.from(node.matrix);
    const [x, y, z, w] = node.rotation || [0, 0, 0, 1];
    const [sx, sy, sz] = node.scale || [1, 1, 1];
    const [tx, ty, tz] = node.translation || [0, 0, 0];
    return Float32Array.from([
      (1 - 2 * (y * y + z * z)) * sx, (2 * (x * y + z * w)) * sx, (2 * (x * z - y * w)) * sx, 0,
      (2 * (x * y - z * w)) * sy, (1 - 2 * (x * x + z * z)) * sy, (2 * (y * z + x * w)) * sy, 0,
      (2 * (x * z + y * w)) * sz, (2 * (y * z - x * w)) * sz, (1 - 2 * (x * x + y * y)) * sz, 0,
      tx, ty, tz, 1]);
  }

  function gltf(json, buffers) {
    const read = (index) => {
      const a = json.accessors[index];
      const view = json.bufferViews[a.bufferView];
      const Type = COMPONENT[a.componentType];
      const n = SIZE[a.type];
      const stride = view.byteStride || n * Type.BYTES_PER_ELEMENT;
      const base = (view.byteOffset || 0) + (a.byteOffset || 0);
      const bytes = buffers[view.buffer];
      const out = new Float32Array(a.count * n);
      const scale = a.normalized ? { 5121: 255, 5123: 65535, 5120: 127, 5122: 32767 }[a.componentType] || 1 : 1;
      for (let i = 0; i < a.count; i++) {
        const row = new Type(bytes.buffer, bytes.byteOffset + base + i * stride, n);
        for (let k = 0; k < n; k++) out[i * n + k] = row[k] / scale;
      }
      return { data: out, size: n };
    };
    const positions = [], colors = [], normals = [], indices = [];
    let vertexCount = 0, anyFaces = false;
    const visit = (nodeIndex, parent) => {
      const node = json.nodes[nodeIndex];
      const matrix = mat4Multiply(parent, nodeMatrix(node));
      if (node.mesh !== undefined) for (const prim of json.meshes[node.mesh].primitives) {
        if (prim.mode !== undefined && prim.mode !== 4 && prim.mode !== 0) continue;
        const p = read(prim.attributes.POSITION).data;
        const count = p.length / 3;
        for (let i = 0; i < count; i++) {
          const [x, y, z] = [p[i * 3], p[i * 3 + 1], p[i * 3 + 2]];
          positions.push(matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12], matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13], matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14]);
        }
        const material = prim.material !== undefined ? (json.materials || [])[prim.material] : null;
        const base = (material && material.pbrMetallicRoughness && material.pbrMetallicRoughness.baseColorFactor) || [0.75, 0.75, 0.75, 1];
        if (prim.attributes.COLOR_0 !== undefined) {
          const c = read(prim.attributes.COLOR_0);
          for (let i = 0; i < count; i++) colors.push(c.data[i * c.size] * base[0], c.data[i * c.size + 1] * base[1], c.data[i * c.size + 2] * base[2]);
        } else for (let i = 0; i < count; i++) colors.push(base[0], base[1], base[2]);
        if (prim.attributes.NORMAL !== undefined) {
          const nrm = read(prim.attributes.NORMAL).data;
          for (let i = 0; i < count; i++) {
            const [x, y, z] = [nrm[i * 3], nrm[i * 3 + 1], nrm[i * 3 + 2]];
            normals.push(matrix[0] * x + matrix[4] * y + matrix[8] * z, matrix[1] * x + matrix[5] * y + matrix[9] * z, matrix[2] * x + matrix[6] * y + matrix[10] * z);
          }
        } else for (let i = 0; i < count; i++) normals.push(0, 0, 0);
        if (prim.mode !== 0) {
          anyFaces = true;
          if (prim.indices !== undefined) read(prim.indices).data.forEach((v) => indices.push(v + vertexCount));
          else for (let i = 0; i < count; i++) indices.push(i + vertexCount);
        }
        vertexCount += count;
      }
      (node.children || []).forEach((child) => visit(child, matrix));
    };
    const identity = Float32Array.from([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
    const scene = (json.scenes || [])[json.scene || 0];
    (scene ? scene.nodes : json.nodes.map((_, i) => i)).forEach((n) => visit(n, identity));
    const hasNormals = normals.some((v) => v !== 0);
    return { positions: new Float32Array(positions), colors: new Float32Array(colors),
             normals: hasNormals ? new Float32Array(normals) : null, indices: anyFaces ? new Uint32Array(indices) : null };
  }

  function glb(buffer) {
    const view = new DataView(buffer);
    if (view.getUint32(0, true) !== 0x46546c67) throw new Error("not a binary glTF");
    let offset = 12, json = null;
    const buffers = [];
    while (offset < view.byteLength) {
      const length = view.getUint32(offset, true), type = view.getUint32(offset + 4, true);
      const chunk = new Uint8Array(buffer, offset + 8, length);
      if (type === 0x4e4f534a) json = JSON.parse(new TextDecoder().decode(chunk));
      else if (type === 0x004e4942) buffers.push(chunk);
      offset += 8 + length;
    }
    return gltf(json, buffers);
  }

  function gltfText(buffer) {
    const json = JSON.parse(new TextDecoder().decode(buffer));
    const buffers = json.buffers.map((b) => {
      if (!b.uri || !b.uri.startsWith("data:")) throw new Error(".gltf with external buffers ; use .glb");
      return Uint8Array.from(atob(b.uri.split(",")[1]), (c) => c.charCodeAt(0));
    });
    return gltf(json, buffers);
  }

  function ply(buffer) {
    const bytes = new Uint8Array(buffer);
    const headerEnd = new TextDecoder().decode(bytes.subarray(0, Math.min(bytes.length, 65536))).indexOf("end_header");
    if (headerEnd < 0) throw new Error("not a ply");
    let bodyStart = headerEnd + "end_header".length;
    while (bytes[bodyStart] === 13 || bytes[bodyStart] === 10) bodyStart++;
    const header = new TextDecoder().decode(bytes.subarray(0, headerEnd)).split(/\r?\n/);
    const format = (header.find((l) => l.startsWith("format")) || "").split(/\s+/)[1];
    const elements = [];
    for (const line of header) {
      const w = line.trim().split(/\s+/);
      if (w[0] === "element") elements.push({ name: w[1], count: Number(w[2]), props: [] });
      else if (w[0] === "property") elements[elements.length - 1].props.push(w[1] === "list" ? { list: true, countType: w[2], type: w[3], name: w[4] } : { type: w[1], name: w[2] });
    }
    const TYPES = { char: ["Int8", 1], int8: ["Int8", 1], uchar: ["Uint8", 1], uint8: ["Uint8", 1], short: ["Int16", 2], int16: ["Int16", 2],
                    ushort: ["Uint16", 2], uint16: ["Uint16", 2], int: ["Int32", 4], int32: ["Int32", 4], uint: ["Uint32", 4], uint32: ["Uint32", 4],
                    float: ["Float32", 4], float32: ["Float32", 4], double: ["Float64", 8], float64: ["Float64", 8] };
    const vertexRows = [], faces = [];
    if (format === "ascii") {
      const tokens = new TextDecoder().decode(bytes.subarray(bodyStart)).trim().split(/\s+/).map(Number);
      let t = 0;
      for (const e of elements) for (let i = 0; i < e.count; i++) {
        const row = {};
        for (const p of e.props) {
          if (p.list) { const n = tokens[t++]; row[p.name] = tokens.slice(t, t + n); t += n; } else row[p.name] = tokens[t++];
        }
        if (e.name === "vertex") vertexRows.push(row); else if (e.name === "face") faces.push(row);
      }
    } else {
      const little = format !== "binary_big_endian";
      const view = new DataView(buffer);
      let o = bodyStart;
      const get = (type) => { const [fn, size] = TYPES[type]; const v = view["get" + fn](o, little); o += size; return v; };
      for (const e of elements) for (let i = 0; i < e.count; i++) {
        const row = {};
        for (const p of e.props) {
          if (p.list) { const n = get(p.countType); const list = new Array(n); for (let k = 0; k < n; k++) list[k] = get(p.type); row[p.name] = list; } else row[p.name] = get(p.type);
        }
        if (e.name === "vertex") vertexRows.push(row); else if (e.name === "face") faces.push(row);
      }
    }
    const positions = new Float32Array(vertexRows.length * 3), colors = new Float32Array(vertexRows.length * 3);
    const clamp = (v) => Math.min(1, Math.max(0, v));
    vertexRows.forEach((v, i) => {
      positions.set([v.x, v.y, v.z], i * 3);
      if ("f_dc_0" in v) colors.set([clamp(0.5 + SH_C0 * v.f_dc_0), clamp(0.5 + SH_C0 * v.f_dc_1), clamp(0.5 + SH_C0 * v.f_dc_2)], i * 3);
      else if ("red" in v) { const s = v.red > 1 || v.green > 1 || v.blue > 1 ? 255 : 1; colors.set([v.red / s, v.green / s, v.blue / s], i * 3); }
      else colors.set([0.75, 0.75, 0.75], i * 3);
    });
    const list = (f) => f.vertex_indices || f.vertex_index || [];
    const indices = [];
    faces.forEach((f) => { const l = list(f); for (let k = 1; k + 1 < l.length; k++) indices.push(l[0], l[k], l[k + 1]); });
    return { positions, colors, normals: null, indices: indices.length ? new Uint32Array(indices) : null };
  }

  function obj(buffer) {
    const vertices = [], vertexColors = [], positions = [], colors = [], indices = [];
    for (const line of new TextDecoder().decode(buffer).split(/\r?\n/)) {
      const w = line.trim().split(/\s+/);
      if (w[0] === "v") { vertices.push(w.slice(1, 4).map(Number)); vertexColors.push(w.length >= 7 ? w.slice(4, 7).map(Number) : [0.75, 0.75, 0.75]); }
      else if (w[0] === "f") {
        const ids = w.slice(1).map((x) => { const i = parseInt(x.split("/")[0], 10); return i < 0 ? vertices.length + i : i - 1; });
        for (let k = 1; k + 1 < ids.length; k++) indices.push(ids[0], ids[k], ids[k + 1]);
      }
    }
    vertices.forEach((v, i) => { positions.push(...v); colors.push(...vertexColors[i]); });
    return { positions: new Float32Array(positions), colors: new Float32Array(colors), normals: null, indices: indices.length ? new Uint32Array(indices) : null };
  }

  function stl(buffer) {
    const view = new DataView(buffer);
    const positions = [];
    const binaryCount = buffer.byteLength >= 84 ? view.getUint32(80, true) : -1;
    if (binaryCount >= 0 && 84 + binaryCount * 50 === buffer.byteLength) {
      for (let i = 0; i < binaryCount; i++) for (let v = 0; v < 3; v++) for (let k = 0; k < 3; k++) positions.push(view.getFloat32(84 + i * 50 + 12 + v * 12 + k * 4, true));
    } else {
      for (const m of new TextDecoder().decode(buffer).matchAll(/vertex\s+(\S+)\s+(\S+)\s+(\S+)/g)) positions.push(Number(m[1]), Number(m[2]), Number(m[3]));
    }
    const count = positions.length / 3;
    return { positions: new Float32Array(positions), colors: new Float32Array(count * 3).fill(0.75), normals: null, indices: Uint32Array.from({ length: count }, (_, i) => i) };
  }

  function splat(buffer) {
    // antimatter15 .splat : 32 bytes per splat , position 3 floats , scale 3 floats , rgba 4 bytes , rotation 4 bytes
    const count = Math.floor(buffer.byteLength / 32);
    const f = new Float32Array(buffer, 0, count * 8), u = new Uint8Array(buffer);
    const positions = new Float32Array(count * 3), colors = new Float32Array(count * 3);
    for (let i = 0; i < count; i++) {
      positions.set([f[i * 8], f[i * 8 + 1], f[i * 8 + 2]], i * 3);
      colors.set([u[i * 32 + 24] / 255, u[i * 32 + 25] / 255, u[i * 32 + 26] / 255], i * 3);
    }
    return { positions, colors, normals: null, indices: null };
  }

  const PARSERS = { glb, gltf: gltfText, ply, obj, stl, splat };

  function smoothNormals(positions, indices) {
    const normals = new Float32Array(positions.length);
    for (let t = 0; t < indices.length; t += 3) {
      const [a, b, c] = [indices[t] * 3, indices[t + 1] * 3, indices[t + 2] * 3];
      const ux = positions[b] - positions[a], uy = positions[b + 1] - positions[a + 1], uz = positions[b + 2] - positions[a + 2];
      const vx = positions[c] - positions[a], vy = positions[c + 1] - positions[a + 1], vz = positions[c + 2] - positions[a + 2];
      const nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
      for (const i of [a, b, c]) { normals[i] += nx; normals[i + 1] += ny; normals[i + 2] += nz; }
    }
    return normals;
  }

  // ------------------------------------------------------------------ rendering
  const VERTEX = `#version 300 es
    in vec3 position; in vec3 color; in vec3 normal;
    uniform mat4 view; uniform mat4 projection; uniform float pointSize;
    out vec3 vColor; out vec3 vNormal;
    void main() {
      vColor = color; vNormal = mat3(view) * normal;
      gl_Position = projection * view * vec4(position, 1.0);
      gl_PointSize = pointSize;
    }`;
  const FRAGMENT = `#version 300 es
    precision mediump float;
    in vec3 vColor; in vec3 vNormal; uniform bool lit; out vec4 outColor;
    void main() {
      float light = 1.0;
      if (lit) { vec3 n = normalize(vNormal); light = 0.35 + 0.65 * abs(n.z); }   // headlight , both faces lit
      outColor = vec4(vColor * light, 1.0);
    }`;

  function perspective(fovy, aspect, near, far) {
    const f = 1 / Math.tan(fovy / 2);
    return Float32Array.from([f / aspect, 0, 0, 0, 0, f, 0, 0, 0, 0, (far + near) / (near - far), -1, 0, 0, (2 * far * near) / (near - far), 0]);
  }

  function create(container) {
    const canvas = document.createElement("canvas");
    canvas.className = "model3d";
    const status = document.createElement("div");
    status.className = "model3d-status";
    container.append(canvas, status);
    const gl = canvas.getContext("webgl2", { antialias: true, preserveDrawingBuffer: true });
    if (!gl) { status.textContent = "3D preview needs WebGL2"; return { load() {}, clear() {} }; }
    const compile = (type, source) => { const s = gl.createShader(type); gl.shaderSource(s, source); gl.compileShader(s); return s; };
    const program = gl.createProgram();
    gl.attachShader(program, compile(gl.VERTEX_SHADER, VERTEX));
    gl.attachShader(program, compile(gl.FRAGMENT_SHADER, FRAGMENT));
    gl.linkProgram(program);
    const where = (name) => gl.getUniformLocation(program, name);
    let model = null, loading = 0;
    const camera = { yaw: 0, pitch: 0, distance: 1, target: [0, 0, 0], home: null };

    function upload(geometry) {
      const buffer = (data, name, size) => {
        const b = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, b);
        gl.bufferData(gl.ARRAY_BUFFER, data, gl.STATIC_DRAW);
        const at = gl.getAttribLocation(program, name);
        gl.enableVertexAttribArray(at);
        gl.vertexAttribPointer(at, size, gl.FLOAT, false, 0, 0);
      };
      const vao = gl.createVertexArray();
      gl.bindVertexArray(vao);
      buffer(geometry.positions, "position", 3);
      buffer(geometry.colors, "color", 3);
      const lit = !!geometry.indices;
      buffer(lit ? (geometry.normals || smoothNormals(geometry.positions, geometry.indices)) : new Float32Array(geometry.positions.length), "normal", 3);
      if (lit) { const ib = gl.createBuffer(); gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, ib); gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, geometry.indices, gl.STATIC_DRAW); }
      gl.bindVertexArray(null);
      // frame the model : its bounding box centre , far enough back to see the whole box
      const p = geometry.positions, lo = [Infinity, Infinity, Infinity], hi = [-Infinity, -Infinity, -Infinity];
      for (let i = 0; i < p.length; i += 3) for (let k = 0; k < 3; k++) { lo[k] = Math.min(lo[k], p[i + k]); hi[k] = Math.max(hi[k], p[i + k]); }
      const radius = Math.max(1e-6, Math.hypot(hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]) / 2);
      camera.home = { target: [0, 1, 2].map((k) => (lo[k] + hi[k]) / 2), distance: radius * 2.6, radius };
      reset();
      return { vao, count: lit ? geometry.indices.length : p.length / 3, lit, radius,
               label: (lit ? geometry.indices.length / 3 + " triangles" : p.length / 3 + " points") };
    }

    function reset() {
      if (!camera.home) return;
      Object.assign(camera, { yaw: 0.6, pitch: 0.35, distance: camera.home.distance, target: camera.home.target.slice() });
      draw();
    }

    function viewMatrix() {
      const cy = Math.cos(camera.yaw), sy = Math.sin(camera.yaw), cp = Math.cos(camera.pitch), sp = Math.sin(camera.pitch);
      const eye = [camera.target[0] + camera.distance * cp * sy, camera.target[1] + camera.distance * sp, camera.target[2] + camera.distance * cp * cy];
      const z = eye.map((v, k) => v - camera.target[k]), zl = Math.hypot(...z);
      const f = z.map((v) => v / zl);
      const x = [f[2], 0, -f[0]], xl = Math.hypot(...x) || 1;   // right = up x f , up = +y
      const r = x.map((v) => v / xl);
      const u = [f[1] * r[2] - f[2] * r[1], f[2] * r[0] - f[0] * r[2], f[0] * r[1] - f[1] * r[0]];
      const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
      return { matrix: Float32Array.from([r[0], u[0], f[0], 0, r[1], u[1], f[1], 0, r[2], u[2], f[2], 0, -dot(r, eye), -dot(u, eye), -dot(f, eye), 1]), right: r, up: u };
    }

    // every change asks for one redraw on the next animation frame : drags and resizes coalesce , and the frame is
    // presented with the page ( a draw outside a frame can stay unpresented until something else repaints )
    let frameRequested = false;
    function draw() {
      if (frameRequested) return;
      frameRequested = true;
      requestAnimationFrame(() => { frameRequested = false; render(); });
    }

    function render() {
      const width = canvas.clientWidth, height = canvas.clientHeight, ratio = window.devicePixelRatio || 1;
      if (canvas.width !== Math.round(width * ratio) || canvas.height !== Math.round(height * ratio)) { canvas.width = Math.round(width * ratio); canvas.height = Math.round(height * ratio); }
      gl.viewport(0, 0, canvas.width, canvas.height);
      const dark = window.matchMedia("(prefers-color-scheme: dark)").matches;
      gl.clearColor(dark ? 0.09 : 0.94, dark ? 0.1 : 0.95, dark ? 0.12 : 0.96, 1);
      gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
      if (!model) return;
      gl.enable(gl.DEPTH_TEST);
      gl.useProgram(program);
      gl.uniformMatrix4fv(where("view"), false, viewMatrix().matrix);
      gl.uniformMatrix4fv(where("projection"), false, perspective(0.8, canvas.width / Math.max(1, canvas.height), camera.distance / 1000, camera.distance + model.radius * 4));
      gl.uniform1f(where("pointSize"), Math.max(1.5, 2 * ratio));
      gl.uniform1i(where("lit"), model.lit ? 1 : 0);
      gl.bindVertexArray(model.vao);
      if (model.lit) gl.drawElements(gl.TRIANGLES, model.count, gl.UNSIGNED_INT, 0); else gl.drawArrays(gl.POINTS, 0, model.count);
      gl.bindVertexArray(null);
    }

    let drag = null;
    canvas.addEventListener("contextmenu", (e) => e.preventDefault());
    canvas.addEventListener("pointerdown", (e) => { drag = { x: e.clientX, y: e.clientY, pan: e.shiftKey || e.button === 2 }; canvas.setPointerCapture(e.pointerId); });
    canvas.addEventListener("pointerup", () => { drag = null; });
    canvas.addEventListener("pointermove", (e) => {
      if (!drag || !model) return;
      const dx = e.clientX - drag.x, dy = e.clientY - drag.y;
      drag.x = e.clientX; drag.y = e.clientY;
      if (drag.pan) {
        const { right, up } = viewMatrix(), s = camera.distance / canvas.clientHeight;
        for (let k = 0; k < 3; k++) camera.target[k] += (-dx * right[k] + dy * up[k]) * s;
      } else {
        camera.yaw -= dx * 0.01;
        camera.pitch = Math.max(-1.55, Math.min(1.55, camera.pitch + dy * 0.01));
      }
      draw();
    });
    canvas.addEventListener("wheel", (e) => { if (!model) return; e.preventDefault(); camera.distance *= Math.exp(e.deltaY * 0.001); draw(); }, { passive: false });
    canvas.addEventListener("dblclick", reset);
    new ResizeObserver(draw).observe(canvas);
    window.matchMedia("(prefers-color-scheme: dark)").addEventListener("change", draw);

    return {
      async load(url, name) {
        const extension = (name || url).split("?")[0].split(".").pop().toLowerCase();
        const ticket = ++loading;
        if (!PARSERS[extension]) { model = null; draw(); status.textContent = "No 3D preview for ." + extension; return; }
        status.textContent = "Loading…";
        try {
          const buffer = await (await fetch(url)).arrayBuffer();
          if (ticket !== loading) return;
          const geometry = PARSERS[extension](buffer);
          if (!geometry.positions.length) throw new Error("no vertices");
          model = upload(geometry);
          status.textContent = model.label;
          canvas.dataset.loaded = model.label;
        } catch (e) {
          model = null; draw();
          status.textContent = "Could not show " + (name || "the model") + ": " + e.message;
        }
      },
      clear() { loading++; model = null; status.textContent = ""; delete canvas.dataset.loaded; draw(); },
    };
  }

  return { create };
})();
