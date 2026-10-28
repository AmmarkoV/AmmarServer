// ASVIO frontend : renders config.json and runs events through /asvio/run.json + /asvio/poll.json ( see PLAN.md )
"use strict";
(function () {
  const FILE_KEY = "__asvio_file__";
  const UPDATE_KEY = "__asvio_update__";
  const session = Array.from(crypto.getRandomValues(new Uint8Array(16)), (b) => b.toString(16).padStart(2, "0")).join("");
  const components = {};   // id -> component object , see make()
  let dependencies = [];

  // ------------------------------------------------------------------ helpers
  function el(tag, attrs, ...children) {
    const node = document.createElement(tag);
    for (const [k, v] of Object.entries(attrs || {})) {
      if (v === null || v === undefined || v === false) continue;
      if (k === "class") node.className = v;
      else if (k.startsWith("on")) node.addEventListener(k.slice(2), v);
      else node.setAttribute(k, v === true ? "" : v);
    }
    for (const child of children) if (child !== null && child !== undefined) node.append(child);
    return node;
  }
  const escapeHtml = (s) => String(s).replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[c]);
  const humanSize = (n) => (n == null ? "" : n < 1024 ? n + " B" : n < 1048576 ? (n / 1024).toFixed(1) + " KB" : (n / 1048576).toFixed(1) + " MB");

  function toast(message) {
    const box = el("div", { class: "toast", role: "alert" }, String(message), el("button", { "aria-label": "Dismiss", onclick: () => box.remove() }, "×"));
    document.getElementById("toasts").append(box);
    setTimeout(() => box.remove(), 15000);
  }

  // Markdown and HTML come from the app author , so HTML is allowed , minus scripts and event handlers
  function sanitize(root) {
    root.querySelectorAll("script,iframe,object,embed").forEach((n) => n.remove());
    root.querySelectorAll("*").forEach((n) => {
      for (const a of Array.from(n.attributes)) {
        if (a.name.startsWith("on") || (/^(href|src|action)$/.test(a.name) && /^\s*javascript:/i.test(a.value))) n.removeAttribute(a.name);
      }
    });
    return root;
  }
  function setHtml(node, html) {
    const t = document.createElement("template");
    t.innerHTML = html;
    sanitize(t.content);
    node.replaceChildren(t.content);
  }
  function inlineMd(s) {
    const codes = [];
    s = s.replace(/`([^`]+)`/g, (_, c) => { codes.push("<code>" + escapeHtml(c) + "</code>"); return "\u0000" + (codes.length - 1) + "\u0000"; });
    s = s.replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>").replace(/__([^_]+)__/g, "<strong>$1</strong>")
         .replace(/(^|[^*])\*([^*\s][^*]*)\*/g, "$1<em>$2</em>")
         .replace(/\[([^\]]+)\]\(([^)\s]+)\)/g, (_, t, u) => '<a href="' + escapeHtml(u) + '" target="_blank" rel="noopener">' + t + "</a>");
    return s.replace(/\u0000(\d+)\u0000/g, (_, i) => codes[i]);
  }
  function markdown(text) {
    const lines = String(text || "").replace(/\r/g, "").split("\n");
    const out = [];
    let i = 0;
    while (i < lines.length) {
      const line = lines[i];
      if (/^```/.test(line)) {
        const code = [];
        for (i++; i < lines.length && !/^```/.test(lines[i]); i++) code.push(lines[i]);
        i++;
        out.push("<pre><code>" + escapeHtml(code.join("\n")) + "</code></pre>");
      } else if (/^#{1,6}\s/.test(line)) {
        const level = line.match(/^#+/)[0].length;
        out.push("<h" + level + ">" + inlineMd(line.slice(level).trim()) + "</h" + level + ">");
        i++;
      } else if (/^\s*([-*+]|\d+\.)\s+/.test(line)) {
        const ordered = /^\s*\d+\./.test(line);
        const items = [];
        for (; i < lines.length && /^\s*([-*+]|\d+\.)\s+/.test(lines[i]); i++) items.push("<li>" + inlineMd(lines[i].replace(/^\s*([-*+]|\d+\.)\s+/, "")) + "</li>");
        out.push((ordered ? "<ol>" : "<ul>") + items.join("") + (ordered ? "</ol>" : "</ul>"));
      } else if (line.trim() === "") {
        i++;
      } else {
        const para = [];
        for (; i < lines.length && lines[i].trim() !== "" && !/^(#{1,6}\s|```|\s*([-*+]|\d+\.)\s+)/.test(lines[i]); i++) para.push(lines[i]);
        const joined = para.join("\n");
        out.push(/^\s*</.test(joined) ? joined : "<p>" + inlineMd(joined) + "</p>");
      }
    }
    return out.join("\n");
  }

  // ------------------------------------------------------------------ file values
  // In the browser a file value is { file: File } ( picked here ) or { ref: "out:<id>" , url } ( from the server )
  function fromWire(v) {
    if (Array.isArray(v)) return v.map(fromWire);
    if (v && typeof v === "object" && FILE_KEY in v) {
      const id = new URLSearchParams(v[FILE_KEY].split("?")[1] || "").get("id");
      return { ref: "out:" + id, url: v[FILE_KEY], name: v.name, size: v.size };
    }
    return v;
  }
  function toWire(v, parts) {
    if (Array.isArray(v)) return v.map((x) => toWire(x, parts));
    if (v && v.file instanceof Blob) {
      // an empty multipart part makes AmmarServer drop it and every part after it , so empty files go by name only
      if (v.file.size === 0) return { [FILE_KEY]: "empty", name: v.file.name };
      const field = "file" + parts.length;
      parts.push([field, v.file]);
      return { [FILE_KEY]: field, name: v.file.name };
    }
    if (v && v.ref) return { [FILE_KEY]: v.ref, name: v.name };
    return v === undefined ? null : v;
  }

  // ------------------------------------------------------------------ components
  // Every component is { node , get() , set(value) , reset() , update(props) , on(event , fn) , fire(event) }
  function base(spec, body, cls) {
    const p = spec.props;
    const listeners = {};
    const node = el("div", { class: "block " + (cls || ""), id: p.elem_id || null });
    if (p.label && p.show_label !== false) node.append(el("label", { class: "title" }, p.label));
    if (p.info) node.append(el("div", { class: "info" }, p.info));
    if (body) node.append(body);
    if (p.visible === false) node.classList.add("hidden");
    return {
      node, spec,
      on(event, fn) { (listeners[event] = listeners[event] || []).push(fn); },
      fire(event) { (listeners[event] || []).forEach((fn) => fn()); },
      update(props) {
        if ("visible" in props) node.classList.toggle("hidden", !props.visible);
        if ("label" in props) { const t = node.querySelector("label.title"); if (t) t.textContent = props.label; }
        if ("interactive" in props) node.querySelectorAll("input,textarea,select").forEach((i) => { i.disabled = !props.interactive; });
        if ("placeholder" in props) node.querySelectorAll("input,textarea").forEach((i) => { i.placeholder = props.placeholder || ""; });
        if ("choices" in props && this.setChoices) this.setChoices(props.choices);
        if ("value" in props) this.set(fromWire(props.value));
      },
      reset() { this.set(fromWire(spec.props.value ?? null)); },
    };
  }

  const make = {
    textbox(spec) {
      const p = spec.props;
      const multi = p.lines > 1;
      const input = multi ? el("textarea", { rows: p.lines, placeholder: p.placeholder }) : el("input", { type: p.type === "password" ? "password" : "text", placeholder: p.placeholder });
      input.disabled = !spec.interactive;
      const c = base(spec, input);
      c.get = () => input.value;
      c.set = (v) => { input.value = v == null ? "" : v; };
      input.addEventListener("input", () => c.fire("change"));
      input.addEventListener("keydown", (e) => { if (e.key === "Enter" && !e.shiftKey && !e.isComposing) { e.preventDefault(); c.fire("submit"); } });
      return c;
    },
    number(spec) {
      const p = spec.props;
      const input = el("input", { type: "number", step: p.precision === 0 ? 1 : (p.step || "any"), min: p.minimum, max: p.maximum });
      input.disabled = !spec.interactive;
      const c = base(spec, input);
      c.get = () => (input.value === "" ? null : Number(input.value));
      c.set = (v) => { input.value = v == null ? "" : v; };
      input.addEventListener("change", () => c.fire("change"));
      return c;
    },
    json(spec) {
      const view = spec.interactive ? el("textarea", { rows: 6 }) : el("pre", { class: "json" });
      const c = base(spec, view);
      let value = null;
      c.get = () => { if (!spec.interactive) return value; try { return view.value.trim() ? JSON.parse(view.value) : null; } catch (e) { throw new Error((spec.props.label || "JSON") + ": " + e.message); } };
      c.set = (v) => { value = v; const text = v == null ? "" : JSON.stringify(v, null, 2); if (spec.interactive) view.value = text; else view.textContent = text; };
      return c;
    },
    markdown(spec, html) {
      const view = el("div", { class: "md" });
      const c = base(spec, view, "prose");
      let value = null;
      c.get = () => value;
      c.set = (v) => { value = v; setHtml(view, html ? (v || "") : markdown(v)); };
      return c;
    },
    html(spec) { return make.markdown(spec, true); },
    button(spec) {
      const button = el("button", { class: spec.props.variant === "primary" ? "primary" : null, type: "button" });
      const c = base(spec, button, "buttonblock");
      c.get = () => button.textContent;
      c.set = (v) => { button.textContent = v == null ? "" : v; };
      button.addEventListener("click", () => c.fire("click"));
      return c;
    },
    clearbutton(spec) {
      const c = make.button(spec);
      c.on("click", () => spec.props.components.forEach((id) => { components[id].reset(); components[id].node.classList.remove("error"); }));
      return c;
    },
    image(spec) { return fileLike(spec, true); },
    file(spec) { return fileLike(spec, false); },
    model3d(spec) {
      // the file list ( download / upload ) plus a WebGL preview of the first file , see model3d.js
      const c = fileLike(spec, false);
      const box = el("div", { class: "model3d-box hidden" });
      c.node.insertBefore(box, c.node.querySelector(".drop"));
      const viewer = window.ASVIOModel3D.create(box);
      let objectUrl = null;
      const show = () => {
        const v = c.get();
        const item = Array.isArray(v) ? v[0] : v;
        if (objectUrl) { URL.revokeObjectURL(objectUrl); objectUrl = null; }
        box.classList.toggle("hidden", !item);
        if (!item) { viewer.clear(); return; }
        const url = item.url || (objectUrl = URL.createObjectURL(item.file));
        viewer.load(url, item.name || item.file.name);
      };
      const set = c.set;
      c.set = (v) => { set(v); show(); };
      c.on("change", show);   // uploads and the clear button
      return c;
    },
    state(spec) {
      // State lives on the server ( per session ) ; here it only holds a place
      const c = base(spec, null);
      c.node.classList.add("hidden");
      c.get = () => null;
      c.set = () => {};
      return c;
    },
    checkbox(spec) {
      const box = el("input", { type: "checkbox" });
      box.disabled = !spec.interactive;
      const c = base(Object.assign({}, spec, { props: Object.assign({}, spec.props, { label: null }) }),
                     el("label", { class: "checkbox" }, box, " ", spec.props.label || ""));
      c.get = () => box.checked;
      c.set = (v) => { box.checked = !!v; };
      box.addEventListener("change", () => c.fire("change"));
      return c;
    },
    slider(spec) {
      const p = spec.props;
      const attrs = { min: p.minimum, max: p.maximum, step: p.step || "any" };
      const range = el("input", Object.assign({ type: "range" }, attrs));
      const number = el("input", Object.assign({ type: "number", class: "slider-number" }, attrs));
      range.disabled = number.disabled = !spec.interactive;
      const c = base(spec, el("div", { class: "slider" }, range, number));
      c.get = () => (number.value === "" ? null : Number(number.value));
      c.set = (v) => { range.value = number.value = v == null ? "" : v; };
      range.addEventListener("input", () => { number.value = range.value; });
      number.addEventListener("input", () => { range.value = number.value; });
      range.addEventListener("change", () => c.fire("change"));
      number.addEventListener("change", () => c.fire("change"));
      return c;
    },
    dropdown(spec) {
      const p = spec.props;
      const select = el("select", { multiple: p.multiselect || null });
      select.disabled = !spec.interactive;
      const c = base(spec, select);
      let choices = [];   // [label , value] , values can be any JSON so they are looked up by index
      c.setChoices = (list) => {
        choices = list.map((x) => (Array.isArray(x) ? x : [String(x), x]));
        select.replaceChildren(...choices.map(([label], i) => el("option", { value: i }, label)));
      };
      c.setChoices(p.choices || []);
      const index = (v) => choices.findIndex(([, value]) => JSON.stringify(value) === JSON.stringify(v));
      c.get = () => {
        const picked = Array.from(select.selectedOptions).map((o) => choices[Number(o.value)][1]);
        return p.multiselect ? picked : (picked.length ? picked[0] : null);
      };
      c.set = (v) => {
        const wanted = new Set((Array.isArray(v) ? v : v == null ? [] : [v]).map(index));
        Array.from(select.options).forEach((o, i) => { o.selected = wanted.has(i); });
      };
      select.addEventListener("change", () => c.fire("change"));
      return c;
    },
    chatbot(spec) {
      const log = el("div", { class: "chat", role: "log" });
      if (spec.props.height) log.style.height = typeof spec.props.height === "number" ? spec.props.height + "px" : spec.props.height;
      const c = base(spec, log);
      let value = [];
      // a message is markdown + HTML , or a file ; turns are { role , content } ( messages ) or [ user , bot ] ( tuples )
      const content = (m) => {
        if (m && typeof m === "object" && FILE_KEY in m) { const f = fromWire(m); return el("a", { href: f.url, download: f.name || "file" }, f.name || "file"); }
        if (m && typeof m === "object" && !Array.isArray(m)) return el("pre", { class: "json" }, JSON.stringify(m, null, 2));
        const box = el("div", { class: "md" });
        setHtml(box, markdown(Array.isArray(m) ? m.join(" ") : m == null ? "" : String(m)));
        return box;
      };
      const bubble = (role, m) => (m == null || m === "" && role === "user" ? null : el("div", { class: "msg " + role }, content(m)));
      c.get = () => value;
      c.set = (v) => {
        value = Array.isArray(v) ? v : [];
        const stick = log.scrollHeight - log.scrollTop - log.clientHeight < 40;
        log.replaceChildren(...value.flatMap((turn) => (Array.isArray(turn)
          ? [bubble("user", turn[0]), bubble("assistant", turn[1])]
          : [bubble(turn.role === "user" ? "user" : "assistant", turn.content)])).filter(Boolean));
        if (stick) {
          log.scrollTop = log.scrollHeight;
          // images load after this and grow the chat , keep the newest message in view
          log.querySelectorAll("img").forEach((img) => { if (!img.complete) img.addEventListener("load", () => { log.scrollTop = log.scrollHeight; }, { once: true }); });
        }
      };
      return c;
    },
    gallery(spec) {
      const p = spec.props;
      const grid = el("div", { class: "gallery", style: "--columns:" + (p.columns || 2) + (p.height ? ";max-height:" + (typeof p.height === "number" ? p.height + "px" : p.height) : "") });
      const c = base(spec, grid);
      let value = null;
      c.get = () => value;
      c.set = (v) => {
        value = v;
        grid.replaceChildren(...(v || []).map(([file, caption]) => {
          const f = fromWire(file);
          return el("figure", {}, el("a", { href: f.url, target: "_blank", rel: "noopener" }, el("img", { src: f.url, alt: caption || f.name || "" })),
                    caption ? el("figcaption", {}, caption) : null);
        }));
      };
      return c;
    },
    examples(spec) {
      // clicking a row fills the inputs in ( and fires their change , as gradio does )
      const p = spec.props;
      const table = el("table", { class: "examples" });
      const c = base(Object.assign({}, spec, { props: Object.assign({}, p, { label: p.label }) }), table);
      const describe = (v) => {
        if (v == null) return "";
        if (Array.isArray(v)) return el("span", {}, ...v.map(describe));
        if (typeof v === "object" && FILE_KEY in v) {
          const f = fromWire(v);
          return /\.(png|jpe?g|gif|webp|bmp)$/i.test(f.name || "") ? el("img", { src: f.url, alt: f.name }) : f.name;
        }
        const text = typeof v === "string" ? v.replace(/<[^>]*>/g, "") : JSON.stringify(v);
        return text.length > 120 ? text.slice(0, 117) + "…" : text;
      };
      c.get = () => null;
      c.set = () => {};
      queueMicrotask(() => {   // the inputs exist once the whole layout is built
        const shown = p.inputs.map((id, i) => [id, i]).filter(([id]) => components[id] && !components[id].node.classList.contains("hidden"));
        table.append(el("tr", {}, ...shown.map(([id]) => el("th", {}, components[id].spec.props.label || ""))));
        p.rows.forEach((row) => table.append(el("tr", { tabindex: 0, onclick: () => fill(row), onkeydown: (e) => { if (e.key === "Enter") fill(row); } },
          ...shown.map(([, i]) => el("td", {}, describe(row[i]))))));
      });
      const fill = (row) => p.inputs.forEach((id, i) => { if (components[id]) { components[id].set(fromWire(row[i])); components[id].fire("change"); } });
      return c;
    },
  };

  // Image and File : a drop zone when interactive , a preview / download list when not
  function fileLike(spec, image) {
    const p = spec.props;
    const multiple = p.file_count === "multiple";
    const drop = el("div", { class: "drop" + (spec.interactive ? "" : " static"), tabindex: spec.interactive ? 0 : null });
    const c = base(spec, drop);
    let value = null;
    let objectUrls = [];
    const picker = el("input", { type: "file", class: "hidden", multiple: multiple || null,
                                 accept: image ? "image/*" : (p.file_types || []).map((t) => (t.startsWith(".") ? t : t + "/*")).join(",") || null });
    drop.append(picker);
    const placeholder = () => (spec.interactive ? (image ? "Drop an image here or click to upload" : "Drop file" + (multiple ? "s" : "") + " here or click to upload") : "");

    function render() {
      objectUrls.forEach(URL.revokeObjectURL);
      objectUrls = [];
      drop.replaceChildren(picker);
      const items = value == null ? [] : Array.isArray(value) ? value : [value];
      if (!items.length) { drop.append(placeholder()); return; }
      const url = (v) => { if (v.url) return v.url; const u = URL.createObjectURL(v.file); objectUrls.push(u); return u; };
      if (image) {
        drop.append(el("img", { src: url(items[0]), alt: p.label || "image" }));
        if (!spec.interactive && items[0].url) c.node.append(c.download = c.download || el("a", { class: "download" }));
        if (c.download) { c.download.href = url(items[0]); c.download.setAttribute("download", items[0].name || "image"); c.download.textContent = "Download"; }
      } else {
        drop.append(el("ul", { class: "files" }, ...items.map((v) => el("li", {},
          v.url ? el("a", { href: v.url, download: v.name || "file" }, v.name || "file") : el("span", {}, v.file.name),
          el("span", { class: "size" }, humanSize(v.size ?? (v.file && v.file.size)))))));
      }
      if (spec.interactive) drop.append(el("button", { class: "clear-x", type: "button", "aria-label": "Clear", onclick: (e) => { e.stopPropagation(); c.set(null); c.fire("change"); } }, "×"));
    }
    function take(fileList) {
      const files = Array.from(fileList).filter((f) => !image || f.type.startsWith("image/"));
      if (!files.length) return;
      value = multiple ? files.map((f) => ({ file: f })) : { file: files[0] };
      render();
      c.fire("change");
      c.fire("upload");
    }
    if (spec.interactive) {
      drop.addEventListener("click", (e) => { if (e.target.tagName !== "A") picker.click(); });
      drop.addEventListener("keydown", (e) => { if (e.key === "Enter" || e.key === " ") { e.preventDefault(); picker.click(); } });
      picker.addEventListener("change", () => { take(picker.files); picker.value = ""; });
      drop.addEventListener("dragover", (e) => { e.preventDefault(); drop.classList.add("over"); });
      drop.addEventListener("dragleave", () => drop.classList.remove("over"));
      drop.addEventListener("drop", (e) => { e.preventDefault(); drop.classList.remove("over"); take(e.dataTransfer.files); });
    }
    c.get = () => value;
    c.set = (v) => { value = v == null || (Array.isArray(v) && !v.length) ? null : v; render(); };
    return c;
  }

  // ------------------------------------------------------------------ layout
  function build(node, config) {
    if (node.type) {   // a container
      const box = el("div", { class: node.type === "blocks" ? "blocks" : node.type, id: node.props.elem_id || null });
      if (node.type === "column") { box.style.setProperty("--scale", node.props.scale ?? 1); box.style.setProperty("--min-width", (node.props.min_width ?? 320) + "px"); }
      if (node.props.visible === false) box.classList.add("hidden");
      // consecutive Tabs share one tab bar , like the Tabs of a gr.Tabs
      let group = null;
      node.children.forEach((child) => {
        if (child.type !== "tab") { group = null; box.append(build(child, config)); return; }
        if (!group) {
          group = { bar: el("div", { class: "tabbar", role: "tablist" }), panels: [] };
          box.append(el("div", { class: "tabs" }, group.bar));
        }
        const panel = build(child, config);
        const g = group;
        const button = el("button", { type: "button", role: "tab", onclick: () => g.panels.forEach(([b, p]) => { b.classList.toggle("selected", b === button); p.classList.toggle("hidden", p !== panel); }) },
                          child.props.label || "Tab " + (g.panels.length + 1));
        if (g.panels.length) panel.classList.add("hidden"); else button.classList.add("selected");
        g.panels.push([button, panel]);
        g.bar.append(button);
        g.bar.parentNode.append(panel);
      });
      return box;
    }
    const spec = config.components[node.id];
    const factory = make[spec.type];
    if (!factory) return el("div", { class: "block error" }, "Unsupported component: " + spec.type);
    const c = factory(spec);
    c.reset();
    components[node.id] = c;
    return c.node;
  }

  // ------------------------------------------------------------------ events
  const running = {};   // dependency id -> Set of job ids

  async function fetchJson(url, options) {
    const response = await fetch(url, options);
    let body = null;
    try { body = await response.json(); } catch (e) { /* not JSON */ }
    if (!response.ok) throw new Error((body && body.error) || "HTTP " + response.status);
    return body;
  }

  function apply(dep, outputs) {
    if (!outputs) return;
    dep.outputs.forEach((id, i) => {
      const c = components[id];
      if (c.spec.type === "state") return;
      const before = JSON.stringify(c.get());
      const v = outputs[i];
      if (v && typeof v === "object" && UPDATE_KEY in v) c.update(v[UPDATE_KEY]);
      else c.set(fromWire(v));
      // gradio fires .change when a function changes a value too ( e.g. clearing a Files input empties its Gallery )
      if (JSON.stringify(c.get()) !== before) c.fire("change");
    });
  }

  function setBusy(dep, status) {
    dep.outputs.forEach((id) => {
      const node = components[id].node;
      node.classList.toggle("busy", !!status);
      if (status) { node.dataset.status = status; node.classList.remove("error"); } else delete node.dataset.status;
    });
  }

  async function run(dep) {
    dep.cancels.forEach((index) => (running[index] || new Set()).forEach((job) => fetch("asvio/cancel.json?job=" + job)));
    if (dep.api_name) {
      const started = Date.now();
      let status = "queued";
      const tick = setInterval(() => setBusy(dep, status + " · " + ((Date.now() - started) / 1000).toFixed(1) + "s"), 100);
      let job = null;
      try {
        const parts = [];
        const data = dep.inputs.map((id) => toWire(components[id].get(), parts));
        const form = new FormData();
        form.append("api", dep.api_name);
        form.append("session", session);
        form.append("data", JSON.stringify(data));   // never empty , see PLAN.md
        parts.forEach(([field, file]) => form.append(field, file, file.name || "upload"));
        job = (await fetchJson("asvio/run.json", { method: "POST", body: form })).job;
        (running[dep.id] = running[dep.id] || new Set()).add(job);
        let seq = 0;
        for (;;) {
          const reply = await fetchJson("asvio/poll.json?job=" + job + "&seq=" + seq);
          // a cancelled job still carries its last streamed payload , which must not overwrite what came after the cancel
          if (reply.seq !== seq && reply.data && reply.status !== "cancelled") {
            if (reply.data.error) throw new Error(reply.data.error);
            apply(dep, reply.data.outputs);
          }
          seq = reply.seq;
          status = reply.status;
          if (status === "done" || status === "cancelled") break;
          if (status === "error") throw new Error("error");
        }
      } catch (e) {
        toast(e.message || e);
        dep.outputs.forEach((id) => components[id].node.classList.add("error"));
      } finally {
        clearInterval(tick);
        setBusy(dep, null);
        if (job && running[dep.id]) running[dep.id].delete(job);
      }
    }
    for (const index of dep.then) await run(dependencies[index]);
  }

  // ------------------------------------------------------------------ start
  fetchJson("config.json").then((config) => {
    document.title = config.title || "ASVIO";
    dependencies = config.dependencies;
    document.getElementById("app").replaceChildren(build(config.layout, config));
    for (const dep of dependencies) {
      for (const [id, event] of dep.targets) if (components[id]) components[id].on(event, () => run(dep));
    }
  }).catch((e) => {
    document.getElementById("app").replaceChildren(el("p", { class: "toast" }, "Could not load the app: " + e.message));
  });
})();
