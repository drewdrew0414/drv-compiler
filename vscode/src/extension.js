'use strict';

const vscode = require('vscode');
const path   = require('path');
const cp     = require('child_process');
const fs     = require('fs');

// ── Keywords / Types / Builtins ────────────────────────────────────────────

const KEYWORDS_CONTROL = [
  'if','else','while','for','return','break','continue','pass',
  'switch','case','default','match','try','catch','throw',
  'in','of','where','otherwise','exists',
  'parallel','simd','async','await','spawn','lazy',
  'static_if','compile_eval','reduction',
  'fence_full','fence_acquire','fence_release',
  'atomic_load','atomic_store','atomic_cas',
];

const KEYWORDS_DECL = [
  'class','trait','impl','extends','module','use','extern','fn',
];

const KEYWORDS_MOD = [
  'public','private','static','abstract','final','synchronized',
  'const','unsigned','mut','override',
];

const KEYWORDS_SPECIAL = [
  'this','Self','null','None','true','false','new','move',
  'Some','Ok','Err','and','or','not','is','as',
];

const TYPES = [
  'int','long','float','double','char','String','boolean','void',
  'var','auto','tensor','list','Map','Union','Option','Result',
  'Own','Ref','Borrow','Self','arg','dim','atomic',
];

const BUILTINS_GLOBAL = [
  'print','input','length','split','abs','max','min','sqrt',
  'floor','ceil','round','sin','cos','tan','fma',
  'sum','mean','dot','norm','std','median',
  'min_index','max_index','avx512_sum','avx512_dot',
  'sort','reverse','range',
  'toUpper','toLower','trim','replace',
  'assert_that','likely','unlikely',
];

const BUILTINS_NS = {
  str:     ['length','split','trim','toUpper','toLower','replace','contains','starts_with','ends_with',
             'left','right','char_code','from_char_code','to_bool','simd_find'],
  lst:     ['push','pop','length','take','slice','contains','sort','reverse'],
  map:     ['set','get','has','remove','keys','values','length'],
  math:    ['abs','pow','sqrt','log','log2','exp','sin','cos','tan','floor','ceil','round','clamp'],
  io:      ['read_file','write_file','file_exists','file_size','list_dir','mmap_read'],
  sys:      ['argv','env','exit','time','sleep','affinity'],
  sync:     ['fence_full','fence_acquire','fence_release','atomic_load','atomic_store','atomic_cas'],
  topology: ['p_cores','e_cores','cache_groups','total','vendor','pin_threads','set_affinity'],
  simd:     ['sum','dot','max','min','mul','add'],
  mem:      ['alloc','free','copy','set'],
  perf:     ['clock','rdtsc'],
  bits:     ['and','or','xor','not','shl','shr','popcount','clz'],
  diff:     ['grad','jacobian'],
  reflect:  ['fields','methods','has_field'],
  wait:     ['tick','seconds','yield'],
};

const ANNOTATIONS = [
  '@bench','@trace','@align','@packed','@layout_soa','@inline','@noinline',
  '@pure','@fastcall','@noalias','@threadsafe','@alloc','@io','@stack',
  '@heap','@local','@region','@warrant','@rebuttal','@defeats',
  '@specialize','@abstract','@unsafe_legacy',
  '@fast_math','@strict_math',
];

// ── Builtin documentation ──────────────────────────────────────────────────

const BUILTIN_DOCS = {
  print:        '`print(args...)` — print to stdout',
  input:        '`input(prompt?)` → String — read from stdin',
  length:       '`length(collection)` → int — collection length',
  sqrt:         '`sqrt(x: float)` → float',
  abs:          '`abs(x)` → absolute value of x',
  sum:          '`sum(arr)` → sum of elements',
  mean:         '`mean(arr)` → arithmetic mean',
  dot:          '`dot(a, b)` → dot product',
  norm:         '`norm(a)` → L2 norm',
  std:          '`std(arr)` → standard deviation',
  median:       '`median(arr)` → median value',
  min_index:    '`min_index(arr)` → index of minimum value',
  max_index:    '`max_index(arr)` → index of maximum value',
  avx512_sum:   '`avx512_sum(arr)` → AVX-512 vectorized sum',
  avx512_dot:   '`avx512_dot(a, b)` → AVX-512 dot product',
  assert_that:  '`assert_that(cond, msg?)` — runtime assertion',
  likely:       '`likely(cond)` — branch prediction hint',
  unlikely:     '`unlikely(cond)` — branch prediction hint',
  sort:         '`sort(arr)` — in-place sort',
  reverse:      '`reverse(arr)` — in-place reverse',
  range:        '`range(n)` / `range(start, end)` → list<int>',
  fma:          '`fma(a, b, c)` → a*b+c (fused multiply-add)',
};

const NS_DOCS = {
  'str.length':        '`str.length(s)` → int',
  'str.split':         '`str.split(s, delim)` → list<String>',
  'str.trim':          '`str.trim(s)` → String',
  'str.toUpper':       '`str.toUpper(s)` → String',
  'str.toLower':       '`str.toLower(s)` → String',
  'str.replace':       '`str.replace(s, from, to)` → String',
  'str.contains':      '`str.contains(s, sub)` → boolean',
  'str.left':          '`str.left(s, n)` → String — leftmost n characters',
  'str.right':         '`str.right(s, n)` → String — rightmost n characters',
  'str.char_code':     '`str.char_code(s, i)` → int — char code at index i',
  'str.from_char_code':'`str.from_char_code(n)` → String',
  'str.simd_find':     '`str.simd_find(s, sub)` → int — SIMD substring search',
  'lst.push':          '`lst.push(arr, val)` — append element',
  'lst.pop':           '`lst.pop(arr)` — remove last element',
  'lst.take':          '`lst.take(arr, n)` → list — first n elements',
  'lst.length':        '`lst.length(arr)` → int',
  'map.set':           '`map.set(m, key, val)` — set key',
  'map.get':           '`map.get(m, key)` → val',
  'map.has':           '`map.has(m, key)` → boolean',
  'io.read_file':      '`io.read_file(path)` → String',
  'io.write_file':     '`io.write_file(path, content)`',
  'io.file_exists':    '`io.file_exists(path)` → boolean',
  'io.list_dir':       '`io.list_dir(path)` → list<String>',
  'io.mmap_read':      '`io.mmap_read(path)` → String — memory-mapped file read',
  'sys.time':          '`sys.time()` → float — Unix timestamp',
  'sys.exit':          '`sys.exit(code)`',
  'sys.argv':          '`sys.argv` — command-line arguments',
  'sys.affinity':      '`sys.affinity(cpu_id)` — pin thread to CPU core',
  'perf.clock':        '`perf.clock()` → float — high-resolution clock',
  'perf.rdtsc':        '`perf.rdtsc()` → long — RDTSC counter',
  'math.pow':          '`math.pow(base, exp)` → float',
  'math.clamp':        '`math.clamp(x, lo, hi)` → clamp x to [lo, hi]',
  'diff.grad':         '`diff.grad(f, x)` → gradient (auto-diff)',
  'diff.jacobian':     '`diff.jacobian(f, x)` → Jacobian matrix',
  'reflect.fields':    '`reflect.fields(obj)` → list<String>',
  'reflect.methods':   '`reflect.methods(obj)` → list<String>',
  'reflect.has_field': '`reflect.has_field(obj, name)` → boolean',
  'wait.tick':         '`wait.tick(n)` — wait n ticks',
  'wait.seconds':      '`wait.seconds(f)` — wait f seconds',
  'wait.yield':        '`wait.yield()` — yield to scheduler',
  // sys.sync
  'sync.fence_full':    '`sys.sync.fence_full()` — full sequential-consistency memory barrier',
  'sync.fence_acquire': '`sys.sync.fence_acquire()` — acquire memory barrier (read fence)',
  'sync.fence_release': '`sys.sync.fence_release()` — release memory barrier (write fence)',
  'sync.atomic_load':   '`sys.sync.atomic_load(x)` → T — atomic read with acquire ordering',
  'sync.atomic_store':  '`sys.sync.atomic_store(x, v)` — atomic write with release ordering',
  'sync.atomic_cas':    '`sys.sync.atomic_cas(x, expected, new)` → boolean — compare-and-swap',
  // sys.topology
  'topology.p_cores':      '`sys.topology.p_cores()` → list<int> — high-performance core IDs',
  'topology.e_cores':      '`sys.topology.e_cores()` → list<int> — efficiency core IDs',
  'topology.cache_groups': '`sys.topology.cache_groups()` → list<list<int>> — cores sharing L3 cache (AMD CCD / Intel ring)',
  'topology.total':        '`sys.topology.total()` → int — total logical CPU count',
  'topology.vendor':       '`sys.topology.vendor()` → String — CPU vendor (Intel / AMD / other)',
  'topology.pin_threads':  '`sys.topology.pin_threads()` — pin calling thread to best L3-sharing core group',
  'topology.set_affinity': '`sys.topology.set_affinity(core_id)` — pin calling thread to specific core',
};

// ── CompletionItemProvider ──────────────────────────────────────────────────

function makeCompletionItem(label, kind, detail, doc) {
  const item = new vscode.CompletionItem(label, kind);
  if (detail) item.detail = detail;
  if (doc)    item.documentation = new vscode.MarkdownString(doc);
  return item;
}

class DriCompletionProvider {
  provideCompletionItems(document, position) {
    const line   = document.lineAt(position).text;
    const prefix = line.substring(0, position.character);

    const items = [];

    // Namespace member completions — triggered after "ns."
    const nsMatch = prefix.match(/\b(str|lst|map|math|io|sys|simd|mem|perf|bits|diff|reflect|wait)\.(\w*)$/);
    if (nsMatch) {
      const ns   = nsMatch[1];
      const fns  = BUILTINS_NS[ns] || [];
      for (const fn of fns) {
        const key = `${ns}.${fn}`;
        const item = makeCompletionItem(
          fn,
          vscode.CompletionItemKind.Function,
          `${ns}.${fn}`,
          NS_DOCS[key] || null
        );
        item.sortText = '0' + fn;
        items.push(item);
      }
      return items;
    }

    // Annotation completions — triggered after '@'
    if (prefix.match(/@\w*$/)) {
      for (const a of ANNOTATIONS) {
        const item = makeCompletionItem(a, vscode.CompletionItemKind.Event, 'annotation');
        item.insertText = a.slice(1); // '@' already typed
        item.sortText = '0' + a;
        items.push(item);
      }
      return items;
    }

    // Global builtin functions
    for (const fn of BUILTINS_GLOBAL) {
      items.push(makeCompletionItem(
        fn,
        vscode.CompletionItemKind.Function,
        'builtin',
        BUILTIN_DOCS[fn] || null
      ));
    }

    // Keywords
    for (const kw of KEYWORDS_CONTROL) {
      items.push(makeCompletionItem(kw, vscode.CompletionItemKind.Keyword, 'control'));
    }
    for (const kw of KEYWORDS_DECL) {
      items.push(makeCompletionItem(kw, vscode.CompletionItemKind.Keyword, 'declaration'));
    }
    for (const kw of KEYWORDS_MOD) {
      items.push(makeCompletionItem(kw, vscode.CompletionItemKind.Keyword, 'modifier'));
    }
    for (const kw of KEYWORDS_SPECIAL) {
      items.push(makeCompletionItem(kw, vscode.CompletionItemKind.Keyword, 'special'));
    }

    // Types
    for (const t of TYPES) {
      items.push(makeCompletionItem(t, vscode.CompletionItemKind.TypeParameter, 'type'));
    }

    // Namespace prefix suggestions
    for (const ns of Object.keys(BUILTINS_NS)) {
      const item = makeCompletionItem(ns, vscode.CompletionItemKind.Module, 'namespace');
      item.insertText = new vscode.SnippetString(`${ns}.\${1}`);
      items.push(item);
    }

    return items;
  }
}

// ── HoverProvider ──────────────────────────────────────────────────────────

class DriHoverProvider {
  provideHover(document, position) {
    const range = document.getWordRangeAtPosition(position, /[@a-zA-Z_][a-zA-Z0-9_.@]*/);
    if (!range) return null;

    const word = document.getText(range);

    // namespace.method form
    if (word.includes('.') && NS_DOCS[word]) {
      return new vscode.Hover(new vscode.MarkdownString(NS_DOCS[word]));
    }

    // Global builtins
    if (BUILTIN_DOCS[word]) {
      return new vscode.Hover(new vscode.MarkdownString(BUILTIN_DOCS[word]));
    }

    // Annotations
    if (word.startsWith('@')) {
      const docs = {
        '@bench':        'Measure execution time of a function or block',
        '@trace':        'Branch tracing — emits Chrome DevTools JSON',
        '@align':        '`@align(N)` — align to N bytes',
        '@packed':       'Remove struct padding (cannot be used with `extends`)',
        '@layout_soa':   'Structure of Arrays (SoA) memory layout',
        '@inline':       'Force inlining',
        '@noinline':     'Prevent inlining',
        '@pure':         'Pure function — no side effects',
        '@noalias':      'No pointer aliasing — all pointer/reference params get `__restrict__`. Static analyzer checks call sites.',
        '@threadsafe':   'Thread-safety guarantee',
        '@region':       '`@region name { }` — arena allocation scope',
        '@warrant':      'Safety guarantee (manually verified)',
        '@rebuttal':     'Safety rebuttal — suppress specific warning',
        '@unsafe_legacy':'Disable safety checks for legacy C FFI',
        '@fast_math':    'Scope-level fast-math: enables `#pragma GCC optimize("fast-math")` for the function. Floating-point associativity assumed — may affect precision.',
        '@strict_math':  'Scope-level strict IEEE 754: enables `#pragma STDC FENV_ACCESS ON`. Required for detecting NaN/Inf exceptions.',
      };
      if (docs[word]) return new vscode.Hover(new vscode.MarkdownString(docs[word]));
    }

    return null;
  }
}

// ── DiagnosticProvider (dri --check) ──────────────────────────────────────

const diagnosticCollection = vscode.languages.createDiagnosticCollection('dri');

function runCheck(document) {
  if (document.languageId !== 'dri') return;

  const config      = vscode.workspace.getConfiguration('dri');
  const compilerPath = config.get('compilerPath', 'dri');
  const filePath    = document.fileName;

  const proc = cp.spawn(compilerPath, [filePath, '--check'], { timeout: 10000 });

  let stderr = '';
  proc.stderr.on('data', d => { stderr += d.toString(); });

  proc.on('close', () => {
    const diags = [];
    const lineRe = /^.*?:(\d+):(\d+):\s*(error|warning):\s*(.*)$/gm;
    let m;
    while ((m = lineRe.exec(stderr)) !== null) {
      const line = Math.max(0, parseInt(m[1], 10) - 1);
      const col  = Math.max(0, parseInt(m[2], 10) - 1);
      const sev  = m[3] === 'error'
        ? vscode.DiagnosticSeverity.Error
        : vscode.DiagnosticSeverity.Warning;
      const msg  = m[4].trim();
      const range = new vscode.Range(line, col, line, col + 1);
      diags.push(new vscode.Diagnostic(range, msg, sev));
    }
    diagnosticCollection.set(document.uri, diags);
  });
}

// ── Shell detection & command building ────────────────────────────────────────
//
// Shell differences on Windows:
//   PowerShell 5.1 : no && operator; needs `& "exe"` call operator for quoted paths
//   PowerShell 7+  : && supported, still needs & call operator
//   CMD            : && supported, no call operator needed
//   Bash/WSL       : && supported, no call operator needed

function getCompilerPath() {
  return vscode.workspace.getConfiguration('dri').get('compilerPath', 'dri');
}

// Detect active terminal shell: 'powershell' | 'ps7' | 'cmd' | 'bash'
function detectShell() {
  const cfg  = vscode.workspace.getConfiguration('dri');
  const pref = cfg.get('shell', 'auto');
  if (pref !== 'auto') return pref;

  const term = vscode.window.activeTerminal;
  if (term) {
    const n = term.name.toLowerCase();
    if (n.includes('cmd') || n.includes('command prompt')) return 'cmd';
    if (n.includes('pwsh') || n.includes('powershell 7'))  return 'ps7';
    if (n.includes('powershell'))                           return 'powershell';
    if (n.includes('bash') || n.includes('wsl') || n.includes('git')) return 'bash';
    if (n.includes('zsh')  || n.includes('fish') || n.includes('sh')) return 'bash';
  }
  return process.platform === 'win32' ? 'powershell' : 'bash';
}

const _q = p => `"${p}"`;

// Build "compile + run" command for each shell
function buildRunCmd(compiler, driFile, exeFile, shell) {
  switch (shell) {
    case 'cmd':
      // CMD: && works natively
      return `${_q(compiler)} ${_q(driFile)} --exe ${_q(exeFile)} && ${_q(exeFile)}`;
    case 'ps7':
      // PowerShell 7: && works, & needed for quoted exe
      return `& ${_q(compiler)} ${_q(driFile)} --exe ${_q(exeFile)} && & ${_q(exeFile)}`;
    case 'powershell':
      // PowerShell 5.1: no &&, use ; if ($LASTEXITCODE -eq 0)
      return `& ${_q(compiler)} ${_q(driFile)} --exe ${_q(exeFile)}; ` +
             `if ($LASTEXITCODE -eq 0) { & ${_q(exeFile)} }`;
    default: // bash / zsh / wsl
      return `${_q(compiler)} ${_q(driFile)} --exe ${_q(exeFile)} && ${_q(exeFile)}`;
  }
}

function buildBuildCmd(compiler, driFile, exeFile, shell) {
  switch (shell) {
    case 'powershell':
    case 'ps7':
      return `& ${_q(compiler)} ${_q(driFile)} --exe ${_q(exeFile)}`;
    default:
      return `${_q(compiler)} ${_q(driFile)} --exe ${_q(exeFile)}`;
  }
}

function buildCheckCmd(compiler, driFile, shell) {
  switch (shell) {
    case 'powershell':
    case 'ps7':
      return `& ${_q(compiler)} ${_q(driFile)} --check`;
    default:
      return `${_q(compiler)} ${_q(driFile)} --check`;
  }
}

// Create or reuse the 'dri' terminal with user-preferred shell
function getOrCreateTerminal(shell) {
  let term = vscode.window.terminals.find(t => t.name === 'dri');
  if (term) return term;

  const opts = { name: 'dri' };
  if (process.platform === 'win32') {
    const cfgShell = vscode.workspace.getConfiguration('dri').get('shell', 'auto');
    const target   = cfgShell === 'auto' ? shell : cfgShell;
    if (target === 'cmd')  opts.shellPath = 'cmd.exe';
    else if (target === 'ps7') opts.shellPath = 'pwsh.exe';
    // default: VSCode's configured default terminal (PS 5.1)
  }
  return vscode.window.createTerminal(opts);
}

function runInTerminal(cmd, shell) {
  const s    = shell || detectShell();
  const term = getOrCreateTerminal(s);
  term.show(true);
  term.sendText(cmd);
}

// ── Command handlers ───────────────────────────────────────────────────────

function cmdRunFile() {
  const editor = vscode.window.activeTextEditor;
  if (!editor || editor.document.languageId !== 'dri') {
    vscode.window.showWarningMessage('Please open a .dri file first.');
    return;
  }
  const filePath = editor.document.fileName;
  const outName  = path.basename(filePath, '.dri');
  const outDir   = path.dirname(filePath);
  const outExe   = path.join(outDir, outName + (process.platform === 'win32' ? '.exe' : ''));
  const compiler = getCompilerPath();
  const shell    = detectShell();

  editor.document.save().then(() => {
    runInTerminal(buildRunCmd(compiler, filePath, outExe, shell), shell);
  });
}

function cmdBuildExe() {
  const editor = vscode.window.activeTextEditor;
  if (!editor || editor.document.languageId !== 'dri') {
    vscode.window.showWarningMessage('Please open a .dri file first.');
    return;
  }
  const filePath = editor.document.fileName;
  const outName  = path.basename(filePath, '.dri');
  const outDir   = path.dirname(filePath);
  const outExe   = path.join(outDir, outName + (process.platform === 'win32' ? '.exe' : ''));
  const compiler = getCompilerPath();
  const shell    = detectShell();

  editor.document.save().then(() => {
    runInTerminal(buildBuildCmd(compiler, filePath, outExe, shell), shell);
  });
}

function cmdCheckSyntax() {
  const editor = vscode.window.activeTextEditor;
  if (!editor || editor.document.languageId !== 'dri') {
    vscode.window.showWarningMessage('Please open a .dri file first.');
    return;
  }
  const filePath = editor.document.fileName;
  const compiler = getCompilerPath();
  const shell    = detectShell();

  editor.document.save().then(() => {
    runInTerminal(buildCheckCmd(compiler, filePath, shell), shell);
  });
}

// ── Activate / Deactivate ───────────────────────────────────────────────────

function activate(context) {
  // Completion provider
  context.subscriptions.push(
    vscode.languages.registerCompletionItemProvider(
      { language: 'dri', scheme: 'file' },
      new DriCompletionProvider(),
      '.', '@'    // trigger characters
    )
  );

  // Hover documentation
  context.subscriptions.push(
    vscode.languages.registerHoverProvider(
      { language: 'dri', scheme: 'file' },
      new DriHoverProvider()
    )
  );

  // Syntax check on save/open
  context.subscriptions.push(
    vscode.workspace.onDidSaveTextDocument(doc => runCheck(doc))
  );
  context.subscriptions.push(
    vscode.workspace.onDidOpenTextDocument(doc => runCheck(doc))
  );

  // Register commands
  context.subscriptions.push(
    vscode.commands.registerCommand('dri.runFile',     cmdRunFile),
    vscode.commands.registerCommand('dri.buildExe',    cmdBuildExe),
    vscode.commands.registerCommand('dri.checkSyntax', cmdCheckSyntax)
  );

  // Check already-open documents
  vscode.workspace.textDocuments.forEach(doc => runCheck(doc));
}

function deactivate() {
  diagnosticCollection.dispose();
}

module.exports = { activate, deactivate };
