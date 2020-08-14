/* eslint-disable */
import {galaxy_txt} from './galaxy.txt'

const reqURL = 'http://127.0.0.1:12345/aliens/send';

let gTable = {};

export function eval_cmd(cmd, state) {
  gTable['_state'] = parse(tokenize(str2cons(state)));

  let node = parse(tokenize(cmd));
  gTable['_internal'] = node;
  let val = node.evaluate();

  // gTable['_internal'] = parse(tokenize('ap ap ap interact galaxy _state ap ap cons'));
  // let val = gTable['_internal'].evaluate();
  gTable['_state'] = parse(tokenize('ap car _internal'));
  let st = gTable['_state'].evaluate();

  let wins = parse(tokenize('ap car ap cdr _internal')).evaluate();

  return [
    wins.pointss,
    list2str(tovallist(st))
  ];
}

export function setup() {
  let lines = galaxy_txt.split('\n');

  lines.forEach (line => {
    let tokens = tokenize(line);
    let varname = tokens[0];
    let stmt = tokens.slice(2);

    gTable[varname] = parse(stmt);
  });

  gTable['_state'] = NilNode();
}

function list2str(elm) {
  if (!Array.isArray(elm)) {
    if (elm === 'nil') return 'nil';
    return elm.toString();
  }

  let res = '';
  for (let i = 0; i < elm.length; i += 1) {
    if (i > 0) res += ' ';
    res += list2str(elm[i]);
  }
  return '[' + res + ']';
}

function str2cons(str) {
  return str.replace(/\[/g, 'ap ap cons ').replace(/]/g, '');
}

const KIND = {
  Number :  'Number',
  Fun :  'Fun',
  App :  'App',
  Mod :  'Mod',
  Draw :  'Draw',
  Draws :  'Draws',
  Unknown :  'Unknown',
}

function stringify(obj) {
  return JSON.stringify(obj, (k, v) => typeof v === "bigint" ? v.toString() + "n" : v)
}

class Node {
  constructor(kind, val) {
    this.kind_ = kind;
    this.name_ = '';
    this.fun_ = null;
    this.arg_ = null;
    this.val_ = val;
  }

  evaluate() {
    if (this.val_) return this.val_;

    if (this.kind_ === KIND.Number) {
      this.val_ = Value.make_val(BigInt(this.name_));
    }
    else if (this.kind_ === KIND.App) {
      let fun = this.fun_.evaluate();
      this.val_ = fun.call(this.arg_).evaluate();
    }
    else if (this.kind_ === KIND.Fun) {
      if (this.name_ === 'inc') {
        this.val_ = new Func1(
          arg => new Node(KIND.Number, Value.make_val(arg.evaluate().val_ + 1n))
        );
      }
      else if (this.name_ === 'dec') {
        this.val_ = new Func1(
          arg => new Node(KIND.Number, Value.make_val(arg.evaluate().val_ - 1n))
        );
      }
      else if (this.name_ === 'add') {
        this.val_ = new Func2(
          (arg1, arg2) => new Node(KIND.Number,
            Value.make_val(arg1.evaluate().val_ + arg2.evaluate().val_))
        )
      }
      else if (this.name_ === 'mul') {
        this.val_ = new Func2(
          (arg1, arg2) => new Node(KIND.Number,
            Value.make_val(arg1.evaluate().val_ * arg2.evaluate().val_))
        )
      }
      else if (this.name_ === 'div') {
        this.val_ = new Func2(
          (arg1, arg2) => new Node(KIND.Number,
            Value.make_val(arg1.evaluate().val_ / arg2.evaluate().val_))
        )
      }
      else if (this.name_ === 'eq') {
        this.val_ = new Func2(
          (arg1, arg2) =>
            arg1.evaluate().val_ === arg2.evaluate().val_
            ? TrueNode() : FalseNode()
        );
      }
      else if (this.name_ === 'lt') {
        this.val_ = new Func2(
          (arg1, arg2) =>
            arg1.evaluate().val_ < arg2.evaluate().val_
            ? TrueNode() : FalseNode()
        );
      }
      else if (this.name_ === 'mod') {
        this.val_ = new Func1(
          arg => arg.evaluate().mod()
        );
      }
      else if (this.name_ === 'dem') {
        this.val_ = new Func1(
          arg => arg.evaluate().dem()
        );
      }
      else if (this.name_ === 'neg') {
        this.val_ = new Func1(
          arg => new Node(KIND.Number, Value.make_val(- arg.evaluate().val_))
        );
      }
      else if (this.name_ === 's') {
        this.val_ = new Func3(
          (arg1, arg2, arg3) => {
            let tmp = Node.make_app(arg2, arg3);
            return arg1.evaluate().call(arg3).evaluate().call(tmp);
          }
        );
      }
      else if (this.name_ === 'c') {
        this.val_ = new Func3(
          (arg1, arg2, arg3) => arg1.evaluate().call(arg3).evaluate().call(arg2)
        );
      }
      else if (this.name_ === 'b') {
        this.val_ = new Func3(
          (arg1, arg2, arg3) => {
            let tmp = Node.make_app(arg2, arg3);
            return arg1.evaluate().call(tmp);
          }
        );
      }
      else if (this.name_ === 't') {
        this.val_ = True();
      }
      else if (this.name_ === 'f') {
        this.val_ = False();
      }
      else if (this.name_ === 'i') {
        this.val_ = new Func1(
          arg => arg
        );
      }
      else if (this.name_ === 'cons') {
        this.val_ = new Func3(
          (arg1, arg2, arg3) => {
            let tmp = Node.make_app(arg3, arg1);
            let tmp2 = Node.make_app(tmp, arg2);
            return tmp2;
          }
        );
      }
      else if (this.name_ === 'car') {
        this.val_ = new Func1(
          arg => Node.make_app(arg, TrueNode())
        );
      }
      else if (this.name_ === 'cdr') {
        this.val_ = new Func1(
          arg => Node.make_app(arg, FalseNode())
        );
      }
      else if (this.name_ === 'nil') {
        this.val_ = Nil();
      }
      else if (this.name_ === 'isnil') {
        this.val_ = new Func1(
          arg => arg.evaluate().is_nil_ ? TrueNode() : FalseNode()
        )
      }
      else if (this.name_ === 'draw') {
        this.val_ = new Func1(
          arg => draw(arg)
        )
      }
      else if (this.name_ === 'multipledraw') {
        this.val_ = new Func1(
          arg => drawlist(arg)
        )
      }
      else if (this.name_ === 'send') {
        this.val_ = new Func1(
          arg => arg.evaluate().send()
        )
      }
      else if (this.name_ === 'if0') {
        this.val_ = new Func3(
          (arg1, arg2, arg3) => arg1.evaluate().val_ === 0n ? arg2 : arg3
        )
      }
      else if (this.name_ === 'interact') {
        this.val_ = new Func3(
          (arg1, arg2, arg3) => {
            while(true) {
              let next = arg1.evaluate().call(arg2).evaluate().call(arg3).evaluate();
              let flag = next.call(TrueNode());
              next = next.call(FalseNode()).evaluate();
              let newState = next.call(TrueNode());
              next = next.call(FalseNode()).evaluate();
              let data = next.call(TrueNode());

              if (flag.evaluate().val_ === 0n) {
                let fst = newState;
                let snd = drawlist(data);

                return tocons([fst, snd]);
              }

              arg2 = newState;
              arg3 = data.evaluate().send();
            }
          }
        );
      }
      else if (this.name_ in gTable) {
        this.val_ = gTable[this.name_].evaluate();
      }
      else {
        throw 'unknown function: ' + this.name_;
      }
    }

    if (this.val_ == null) {
      throw 'cannot evaluate'
    }

    return this.val_;
  }

  static make_app(fun, arg) {
    let res = new Node(KIND.App, null);
    res.fun_ = fun;
    res.arg_ = arg;

    return res;
  }
}

class Value {
  constructor() {
    this.val_ = 0n;
    this.mod_ = '';
    this.is_val_ = false;
    this.is_nil_ = false;
    this.is_mod_ = false;
    this.name_ = '';
  }

  static make_val(val) {
    let obj = new Value();
    obj.is_val_ = true;
    obj.val_ = val;
    return obj;
  }

  static make_nil() {
    let obj = new Value();
    obj.is_nil_ = true;
    return obj;
  }

  static make_mod(mod) {
    let obj = new Value();
    obj.is_mod_ = true;
    obj.mod_ = mod;
    return obj;
  }

  send () {
    let msg = this.mod().evaluate().mod_;
    let req = new XMLHttpRequest();
    req.open('POST', reqURL, false);
    // req.setRequestHeader('content-type',
    //   'application/x-www-form-urlencoded;charset=UTF-8');
    req.send(msg);

    let i = { n : 0 };
    return Value.dem_str(req.responseText, i);
  }

  call () {
    throw "not callable";
  }

  mod () {
    if (this.is_val_) {
      let mod = '';
      let val = this.val_;

      if (val >= 0n) {
        mod += '01';
      }
      else {
        mod += '10';
        val = -val;
      }

      let bit = 0n, ub = 1n;
      while (val >= ub) {
        ub *= 16n;
        bit += 1n;
      }

      mod += '1'.repeat(Number(bit)) + '0';
      ub /= 2n;
      while(ub > 0n) {
        mod += (val & ub) ? '1' : '0';
        ub /= 2n;
      }

      return new Node(KIND.Mod, Value.make_mod(mod));
    }

    let mod = '';
    if (this.is_nil_) {
      mod += '00';
    }
    else {
      let hd = this.call(TrueNode()).evaluate();
      let tl = this.call(FalseNode()).evaluate();

      mod += '11';
      mod += hd.mod().evaluate().mod_;
      mod += tl.mod().evaluate().mod_;
    }

    return new Node(KIND.Mod, Value.make_mod(mod));
  }

  static dem_str(mod, i) {
    let sig = mod.substring(i.n, i.n + 2);
    
    if (sig === '01' || sig === '10') {
      let val = 0n;
      let sign = (sig === '01' ? 1n : -1n);

      let bit = 0n;
      i.n += 2;
      while (true) {
        if (mod[i.n] === '0') break;
        bit += 1n;
        i.n += 1;
      }
      i.n += 1;

      let p = 1n;
      for (let j = 0n; j < bit * 4n - 1n; j += 1n) p *= 2n;
      let len = bit * 4n;
      for (let j = 0n; j < len; j += 1n) {
        val += p * (mod[i.n] == '1' ? 1n : 0n);
        i.n += 1;
        p /= 2n;
      }
      val *= sign;

      return new Node(KIND.Number, Value.make_val(val));
    }

    i.n += 2;
    if (sig === '00') {
      return NilNode();
    }

    let hd = Value.dem_str(mod, i);
    let tl = Value.dem_str(mod, i);

    return new Func1Node(
      arg => arg.evaluate().call(hd).evaluate().call(tl)
    );
  }

  dem () {
    let i = { n : 0 };
    return Value.dem_str(this.mod_, i);
  }
}

class Func1 extends Value {
  constructor(f) {
    super();
    this.f_ = f;
  }
  call (arg) {
    return this.f_(arg);
  }
}
class Func1Node extends Node {
  constructor(f) {
    super(KIND.Fun, new Func1(f));
  }
}

class Func2 extends Value {
  constructor(f) {
    super();
    this.f_ = f;
  }
  call (arg1) {
    return new Func1Node(
      arg2 => this.f_(arg1, arg2)
    );
  }
}
class Func2Node extends Node {
  constructor(f) {
    super(KIND.Fun, new Func2(f));
  }
}

class Func3 extends Value {
  constructor(f) {
    super();
    this.f_ = f;
  }
  call (arg1) {
    return new Func2Node(
      (arg2, arg3) => this.f_(arg1, arg2, arg3)
    );
  }
}
class Func3Node extends Node {
  constructor(f) {
    super(KIND.Fun, new Func3(f));
  }
}

function True() {
  return new Func2(
    (arg1, arg2) => arg1
  );
}
function TrueNode() {
  return new Node(KIND.Fun, True());
}

function False() {
  return new Func2(
    (arg1, arg2) => arg2
  );
}
function FalseNode() {
  return new Node(KIND.Fun, False());
}

function Nil() {
  let res = new Func1(arg => TrueNode());
  res.is_nil_ = true;
  return res;
}
function NilNode() {
  return new Node(KIND.Nil, Nil());
}

class Draw extends Value {
  constructor() {
    super();
    this.points = [];
  }
}

function draw(node) {
  let val = node.evaluate();
  let res = new Draw();

  while (true) {
    if (val.is_nil_) break;

    let point = val.call(TrueNode()).evaluate();
    let x = point.call(TrueNode()).evaluate().val_;
    let y = point.call(FalseNode()).evaluate().val_;
    res.points.push([x, y]);
    val = val.call(FalseNode()).evaluate();
  }

  return new Node(KIND.Draw, res);
}

class DrawList extends Value {
  constructor() {
    super();
    this.pointss = [];
  }
}

function drawlist(node) {
  let val = node.evaluate();
  let res = new DrawList();

  while (true) {
    if (val.is_nil_) break;

    let points = val.call(TrueNode());
    let tmp = draw(points).evaluate()

    res.pointss.push(tmp.points);
    val = val.call(FalseNode()).evaluate();
  }

  return new Node(KIND.DrawList, res);
}

function tocons(nodes) {
  let res = NilNode();

  for (let i = nodes.length - 1; i >= 0; i -= 1) {
    let ap1 = new Node(KIND.App, null);
    let ap2 = new Node(KIND.App, null);

    ap2.fun_ = new Node(KIND.Fun, null);
    ap2.fun_.name_ = 'cons';
    ap2.arg_ = nodes[i];
    ap1.fun_ = ap2;
    ap1.arg_ = res;

    res = ap1;
  }

  return res;
}

function tovallist(val) {
  if (val.is_nil_) {
    return 'nil';
  }
  if (val.is_val_) {
    return val.val_;
  }

  let a = val.call(TrueNode()).evaluate();
  let b = val.call(FalseNode()).evaluate();

  return [tovallist(a), tovallist(b)];
}

function tokenize(str) {
  return str.split(' ');
}

function parse_(tokens, n) {
  if (tokens.length === n.n) return null;
  const token = tokens[n.n];
  n.n += 1;

  if (token === '') {
    return parse_(tokens, n);
  }

  try {
    let n = BigInt(token);
    return new Node(KIND.Number, Value.make_val(n));
  } catch {}

  if (token === 'ap') {
    let res = new Node(KIND.App, null);
    res.fun_ = parse_(tokens, n);
    res.arg_ = parse_(tokens, n);

    return res;
  }

  let res = new Node(KIND.Fun, null);
  res.name_ = token;
  return res;
}

function parse(tokens) {
  let i = { n : 0 };
  return parse_(tokens, i);
}
