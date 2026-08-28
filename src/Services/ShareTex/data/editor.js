(function () {
  'use strict';

  var sessionID   = document.getElementById('sessionID').value;
  var projectID   = document.getElementById('projectID').value;
  var activeFile  = document.getElementById('activeFile').value;
  var currentVersion = parseInt(document.getElementById('initialVersion').value, 10) || 0;

  var textarea    = document.getElementById('editorTextarea');
  var mirror      = document.getElementById('editorMirror');
  var saveStatus  = document.getElementById('saveStatus');
  var cursorStatus= document.getElementById('cursorStatus');
  var compileLog  = document.getElementById('compileLog');
  var pdfFrame    = document.getElementById('pdfFrame');
  var compileBtn  = document.getElementById('compileBtn');
  var editorBody  = document.querySelector('.editorBody');

  var dirty = false;
  var saveTimer = null;

  var COLOR_PALETTE = ['#e74c3c', '#27ae60', '#8e44ad', '#f39c12', '#16a085', '#2980b9', '#c0392b', '#d35400'];
  function colorForUser(name) {
    var hash = 0;
    for (var i = 0; i < name.length; i++) { hash = (hash * 31 + name.charCodeAt(i)) >>> 0; }
    return COLOR_PALETTE[hash % COLOR_PALETTE.length];
  }

  function urlEncode(obj) {
    var parts = [];
    for (var k in obj) { if (obj.hasOwnProperty(k)) { parts.push(encodeURIComponent(k) + '=' + encodeURIComponent(obj[k])); } }
    return parts.join('&');
  }

  function httpGet(url, callback) {
    var x = new XMLHttpRequest();
    x.open('GET', url, true);
    x.onreadystatechange = function () {
      if (x.readyState === 4) { callback(x.status === 200 ? x.responseText : null); }
    };
    x.send(null);
  }

  // The server only parses multipart/form-data POST bodies ( confirmed against the live server while building
  // this : a plain application/x-www-form-urlencoded body is silently ignored ) , so every AJAX POST here uses
  // FormData , same as a real <form enctype="multipart/form-data"> submit , rather than a urlencoded string.
  function httpPost(url, fields, callback) {
    var fd = new FormData();
    for (var k in fields) { if (fields.hasOwnProperty(k)) { fd.append(k, fields[k]); } }
    var x = new XMLHttpRequest();
    x.open('POST', url, true);
    x.onreadystatechange = function () {
      if (x.readyState === 4) { callback(x.status === 200 ? x.responseText : null); }
    };
    x.send(fd);
  }

  function escapeForMirror(text) {
    return text.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  }

  // ---------------------------------------------------------------- saving
  function doSave() {
    dirty = false;
    saveStatus.textContent = 'Saving...';
    httpPost('saveFileContent.html', { s: sessionID, project: projectID, file: activeFile, content: textarea.value, baseVersion: currentVersion }, function (resp) {
      if (resp == null) { saveStatus.textContent = 'Save failed'; return; }
      var parts = resp.split(' ');
      var newVersion = parseInt(parts[0], 10);
      var conflict = parts[1] === '1';
      if (newVersion > 0) { currentVersion = newVersion; }
      saveStatus.textContent = conflict ? 'Saved (a collaborator also just saved this file)' : 'Saved';
    });
  }

  function scheduleSave() {
    dirty = true;
    saveStatus.textContent = 'Editing...';
    clearTimeout(saveTimer);
    saveTimer = setTimeout(doSave, 2000);
  }

  textarea.addEventListener('input', scheduleSave);
  window.addEventListener('beforeunload', function () { if (dirty) { doSave(); } });

  // ------------------------------------------------------------ poll content
  function pollContent() {
    if (document.activeElement === textarea) { return; } // never clobber active typing
    httpGet('getFileContent.html?' + urlEncode({ s: sessionID, project: projectID, file: activeFile, _: Date.now() }), function (resp) {
      if (resp == null) { return; }
      var idx = resp.indexOf('\n');
      if (idx < 0) { return; }
      var m = resp.substring(0, idx).match(/###VERSION (\d+)###/);
      if (!m) { return; }
      var serverVersion = parseInt(m[1], 10);
      if ((serverVersion > currentVersion) && (!dirty)) {
        textarea.value = resp.substring(idx + 1);
        currentVersion = serverVersion;
      }
    });
  }

  // ------------------------------------------------------------- cursors
  function postCursorPosition() {
    httpPost('postCursor.html', { s: sessionID, project: projectID, file: activeFile, offset: textarea.selectionStart }, function () {});
  }

  function measureOffset(offset) {
    var text = textarea.value;
    var before = escapeForMirror(text.substring(0, Math.min(offset, text.length)));
    mirror.innerHTML = before + '<span id="__marker"></span>';
    var marker = document.getElementById('__marker');
    if (!marker) { return null; }
    return { top: marker.offsetTop, left: marker.offsetLeft };
  }

  function clearRemoteCarets() {
    var existing = editorBody.querySelectorAll('.remoteCaret');
    for (var i = 0; i < existing.length; i++) { existing[i].parentNode.removeChild(existing[i]); }
  }

  function pollCursors() {
    httpGet('pollCursors.html?' + urlEncode({ s: sessionID, project: projectID, file: activeFile, _: Date.now() }), function (resp) {
      clearRemoteCarets();
      if (resp == null) { return; }
      var lines = resp.split('\n').filter(function (l) { return l.length > 0; });
      var names = [];
      lines.forEach(function (line) {
        var parts = line.split('|');
        var name = parts[0];
        var offset = parseInt(parts[1], 10);
        names.push(name);

        var pos = measureOffset(offset);
        if (!pos) { return; }
        var color = colorForUser(name);

        var caret = document.createElement('div');
        caret.className = 'remoteCaret';
        caret.style.top = (pos.top - textarea.scrollTop) + 'px';
        caret.style.left = (pos.left - textarea.scrollLeft) + 'px';
        caret.style.height = '18px';
        caret.style.background = color;

        var label = document.createElement('div');
        label.className = 'remoteCaretLabel';
        label.textContent = name;
        label.style.background = color;
        caret.appendChild(label);

        editorBody.appendChild(caret);
      });
      cursorStatus.textContent = names.length > 0 ? ('Also here: ' + names.join(', ')) : '';
    });
  }

  textarea.addEventListener('scroll', function () { mirror.scrollTop = textarea.scrollTop; mirror.scrollLeft = textarea.scrollLeft; });

  // ------------------------------------------------------------- compile
  function doCompile() {
    compileBtn.disabled = true;
    compileBtn.textContent = 'Compiling...';
    compileLog.textContent = 'Compiling, please wait (this can take up to a minute)...';

    if (dirty) { clearTimeout(saveTimer); doSave(); }

    httpPost('compile.html', { s: sessionID, project: projectID }, function (resp) {
      compileBtn.disabled = false;
      compileBtn.textContent = 'Compile';
      if (resp == null) { compileLog.textContent = 'Compile request failed.'; return; }

      var idx = resp.indexOf('\n');
      var header = idx >= 0 ? resp.substring(0, idx) : resp;
      var log = idx >= 0 ? resp.substring(idx + 1) : '';

      var okMatch = header.match(/###OK (.+)###/);
      if (okMatch) {
        pdfFrame.src = buildPdfURL(okMatch[1]);
        compileLog.textContent = 'Compiled successfully.\n\n' + log;
      } else {
        compileLog.textContent = 'Compile failed.\n\n' + log;
      }
    });
  }

  function buildPdfURL(pdfFilename) {
    return 'projects/' + projectID + '/files/' + pdfFilename;
  }

  compileBtn.addEventListener('click', doCompile);

  // ------------------------------------------------------------- new-file form via normal POST (progressive enhancement not required)

  // ------------------------------------------------------------- ticking
  setInterval(pollContent, 2000);
  setInterval(function () { postCursorPosition(); pollCursors(); }, 1500);
})();
