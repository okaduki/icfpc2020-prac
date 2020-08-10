#include <bits/stdc++.h>
#include <curl/curl.h>

using namespace std;

enum class Kind {
  Number,
  Fun,
  App,
  Mod,
  Draw,
  Draws,
  Unknown,
};

struct Func;
struct Node;
struct Object;
using FuncPtr = shared_ptr<Func>;
using NodePtr = shared_ptr<Node>;
using ObjectPtr = shared_ptr<Object>;

struct Object {
protected:
  long val_;
  string mod_;
  bool is_val_;
  bool is_nil_;
  bool is_mod_;
  string name_;

public:
  Object() : val_(0), is_val_(false), is_nil_(false), is_mod_(false) {}
  Object(long val) : val_(val), is_val_ (true), is_nil_(false), is_mod_(false) {}

  static ObjectPtr make() { return make_shared<Object>(); }
  static ObjectPtr make(long val) { return make_shared<Object>(val); }

  long val() const {
    if (!is_val_) {
      throw runtime_error("not value");
    }
    return val_;
  }
  string mod_val() const {
    if (!is_mod_) {
      throw runtime_error("not mod");
    }
    return mod_;
  }
  string name() const { return is_val_ ? "val:" + to_string(val_) : is_nil_ ? "nil" : name_; }

  bool is_val() const { return is_val_; }
  bool is_nil() const { return is_nil_; }
  bool is_mod() const { return is_mod_; }

  virtual NodePtr call(NodePtr arg) const {
    throw runtime_error("object is not callable");
  }

  NodePtr send();

  virtual void dump() const;

  NodePtr mod();
  NodePtr dem();
};

struct Node {
  Node(Kind kind = Kind::Unknown, ObjectPtr val = nullptr);
  static NodePtr make(Kind kind = Kind::Unknown, ObjectPtr val = nullptr) {
    return make_shared<Node>(kind, val);
  }

  void dump(int indent = 0) const;
  ObjectPtr eval();

  Kind kind_;
  string name_;
  NodePtr fun_;
  NodePtr arg_;
  ObjectPtr val_;
};

struct Func1 : public Object {
  using Type = function<NodePtr(NodePtr)>;
  static shared_ptr<Func1> make(Type f, string name) { return make_shared<Func1>(f, name); }
  Type f_;
  Func1 (Type f, string name) : Object(), f_(f) { name_ = name; }
  virtual NodePtr call(NodePtr obj) const override {
    return f_(obj);
  }
};
struct Func1Node : public Node {
  Func1Node(shared_ptr<Func1> val) : Node(Kind::Fun, val) { }
  static shared_ptr<Func1Node> make(Func1::Type f, string name) { return make_shared<Func1Node>( make_shared<Func1>(f, name)); }
};

struct Func2 : public Object {
  using Type = function<NodePtr(NodePtr,NodePtr)>;
  static shared_ptr<Func2> make(Type f, string name) { return make_shared<Func2>(f, name); }
  Type f_;
  Func2 (Type f, string name) : Object(), f_(f) { name_ = name; }
  virtual NodePtr call(NodePtr arg1) const override {
    auto f = f_;
    return Func1Node::make(
      [f,arg1](NodePtr arg2) {
        return f(arg1, arg2);
      },
      name_ + "(" + arg1->name_ + ")"
    );
  }
};
struct Func2Node : public Node {
  Func2Node(shared_ptr<Func2> val) : Node(Kind::Fun, val) {}
  static shared_ptr<Func2Node> make(Func2::Type f, string name) { return make_shared<Func2Node>(make_shared<Func2>(f, name)); }
};

struct Func3 : public Object {
  using Type = function<NodePtr(NodePtr,NodePtr,NodePtr)>;
  static shared_ptr<Func3> make(Type f, string name) { return make_shared<Func3>(f, name); }
  Type f_;
  Func3 (Type f, string name) : Object(), f_(f) { name_ = name; }
  virtual NodePtr call(NodePtr arg1) const override {
    auto f = f_;
    return Func2Node::make(
      [f,arg1](NodePtr arg2, NodePtr arg3) {
        return f(arg1, arg2, arg3);
      }
      , name_ + "(" + arg1->name_ + ")"
    );
  }
};
struct Func3Node : public Node {
  Func3Node(shared_ptr<Func3> val) : Node(Kind::Fun, val) {}
  static shared_ptr<Func3Node> make(Func3::Type f, string name) { return make_shared<Func3Node>(make_shared<Func3>(f, name)); }
};

ObjectPtr True () {
  return make_shared<Func2> (
    [&](NodePtr arg1, NodePtr arg2) {
      return arg1;
    }, "t"
  );
}
NodePtr TrueNode() {
  NodePtr node = Node::make(Kind::Fun, True());
  node->name_ = "t";
  return node;
};

ObjectPtr False () {
  return make_shared<Func2> (
    [&](NodePtr arg1, NodePtr arg2) {
      return arg2;
    }, "f"
  );
}
NodePtr FalseNode() {
  NodePtr node = Node::make(Kind::Fun, False());
  node->name_ = "f";
  return node;
};

struct Nil : public Object {
  Nil() : Object() { is_nil_ = true; }
  virtual NodePtr call(NodePtr obj) const override {
    return TrueNode();
  }
};
struct NilNode : public Node {
  NilNode() : Node(Kind::Fun, make_shared<Nil>()) { }
};

NodePtr Object::mod() {
  if (is_val_) {
    ObjectPtr res = Object::make();
    res->is_mod_ = true;

    string mod = "";
    long val = val_;

    if (val >= 0) {
      mod += "01";
    }
    else {
      mod += "10";
      val = -val;
    }

    long bit = 0, ub = 1;
    while(val >= ub) {
      ub <<= 4;
      ++bit;
    }

    mod += string(bit, '1') + "0";
    ub >>= 1;
    while(ub) {
      mod += (val & ub) ? "1" : "0";
      ub >>= 1;
    }

    res->mod_ = mod;
    return Node::make(Kind::Mod, res);
  }
  else {
    string mod;
    if (is_nil()) {
      mod += "00";
    }
    else {
      auto hd = this->call(TrueNode())->eval();
      auto tl = this->call(FalseNode())->eval();

      mod += "11";
      mod += hd->mod()->eval()->mod_;
      mod += tl->mod()->eval()->mod_;
    }

    ObjectPtr res = Object::make();
    res->mod_ = mod;
    res->is_mod_ = true;
    return Node::make(Kind::Mod, res);
  }
}

NodePtr dem_str(const string& mod, int& ix) {
  string sig = mod.substr(ix, 2);

  if (sig == "01" || sig == "10") { // number
    long val = 0, sign = 0;
    if (sig == "01") sign = 1;
    else if (sig == "10") sign = -1;

    long bit = 0;
    ix += 2;
    for(;;++ix){
      if (mod[ix] == '0') break;
      bit += 1;
    }
    ++ix;

    long p = (1l << (bit * 4 - 1));
    long len = bit * 4;
    for(int i=0; i < len; ++i) {
      val += p * (mod[ix++] == '1' ? 1 : 0);
      p >>= 1;
    }

    val = sign * val;
    auto res = Object::make(val);
    return Node::make(Kind::Number, res);
  }
  else { // list
    ix += 2;
    if (sig == "00") {
      return make_shared<NilNode>();
    }

    auto hd = dem_str(mod, ix);
    auto tl = dem_str(mod, ix);

    return Func1Node::make([=](NodePtr arg1){
      return arg1->eval()->call(hd)->eval()->call(tl);
    }, "dem_inner");
  }
}

NodePtr Object::dem() {
  int i = 0;
  return dem_str(mod_, i);
}

struct Draw : public Object {
  vector<pair<long,long>> points;
  Draw() : Object() { name_ = "draw()"; }

  void dump() const override {
    cerr << "(";
    for(const auto& p : points) {
      cerr << "(" << p.first << " " << p.second << ")";
    }
    cerr << ")" << endl;
  }
};

void Object::dump() const {
  if (this->is_nil()){
    cerr << "nil";
    return;
  }
  if (this->is_val()) {
    cerr << this->val() << " ";
    return;
  }
  else if (this->is_mod()) {
    cerr << "[" << this->mod_val() << "]";
    return;
  }

  // if (this->name_.substr(0, 4) == "cons") {
  if (1) {
    cerr << "(";

    auto hd = this->call(TrueNode())->eval();
    hd->dump();
    cerr << ", ";

    auto tl = this->call(FalseNode())->eval();
    tl->dump();

    cerr << ")";
  }
  else {
    cerr << this->name();
  }
}

struct DrawList : public Object {
  vector<vector<pair<long,long>>> pointss;
  DrawList() : Object() { name_ = "multipledraw()"; }

  void dump() const override {
    /*
    cerr << "(";
    for(const auto& points : pointss) {
      cerr << "(";
      for(const auto & p : points)
        cerr << "(" << p.first << " " << p.second << ")";
      cerr << ") ";
    }
    cerr << ")" << endl;
    */

    ofstream ofs("windows.pgm");
    long mn_x = 1e9, mn_y = 1e9;
    long mx_x = -1e9, mx_y = -1e9;
    for(const auto& ps : pointss) {
      for(const auto& p : ps) {
        mn_x = min(mn_x, p.first);
        mx_x = max(mx_x, p.first);
        mn_y = min(mn_y, p.second);
        mx_y = max(mx_y, p.second);
      }
    }

    int w = mx_x - mn_x + 1;
    int h = mx_y - mn_y + 1;
    ofs << "P2" << endl
        << w << " " << h << endl
        << 255 << endl;
    
    vector<vector<long>> dat(h, vector<long>(w, 0));

    long delta = 255 / pointss.size();
    long col = 0;
    for(const auto& ps : pointss) {
      col += delta;
      for(const auto& p : ps) {
        if (dat[p.second - mn_y][p.first - mn_x] == 0)
          dat[p.second - mn_y][p.first - mn_x] = col;
      }
    }
    cerr << mn_x <<","<<mn_y<<endl;

    for(auto& rows : dat) {
      for(auto& val : rows) ofs << val << " ";
      ofs << endl;
    }
  }
};

NodePtr draw(NodePtr node) {
  auto obj = node->eval();
  auto res = make_shared<Draw>();

  while(true){
    if (obj->is_nil()) break;

    auto point = obj->call(TrueNode())->eval();
    auto x = point->call(TrueNode())->eval()->val();
    auto y = point->call(FalseNode())->eval()->val();

    res->points.emplace_back(x, y);
    obj = obj->call(FalseNode())->eval();
  }

  return Node::make(Kind::Draw, res);
}

NodePtr drawlist(NodePtr node) {
  auto obj = node->eval();
  auto res = make_shared<DrawList>();
  while(true){
    if (obj->is_nil()) break;

    auto points = obj->call(TrueNode());
    auto tmp = dynamic_pointer_cast<Draw>(draw(points)->eval());

    res->pointss.emplace_back(tmp->points);
    obj = obj->call(FalseNode())->eval();
  }

  return Node::make(Kind::Draws, res);
}

size_t send_callback(char *ptr, size_t size, size_t nmemb, string *stream) {
  int dataLength = size * nmemb;
  stream->append(ptr, dataLength);
  return dataLength;
}

string send(const string& str) {
  CURL *curl = curl_easy_init();

  string resp;
  curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:12345/aliens/send");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, send_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);

  curl_easy_setopt(curl, CURLOPT_POST, 1);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, str.size());

  CURLcode ret = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (ret != CURLE_OK) {
    cerr << "curl failed" << endl;
  }
  return resp;
};

NodePtr Object::send() {
  auto str = this->mod()->eval()->mod_val();
  string resp = ::send(str);

  auto obj = Object::make();
  obj->is_mod_ = true;
  obj->mod_ = resp;

  return obj->dem();
}

Node::Node(Kind kind, ObjectPtr val) : kind_(kind), val_(val) {}

void Node::dump(int indent) const {
  cerr << string(indent, ' ');
  if (kind_ == Kind::Number) {
    cerr << "Num " << name_ << endl;
  }
  else if (kind_ == Kind::Fun) {
    cerr << "Fun " << name_ << endl;
  }
  else if (kind_ == Kind::App) {
    cerr << "App" << endl;
    fun_->dump(indent + 2);
    arg_->dump(indent + 2);
  }
  else if (kind_ == Kind::Mod) {
    cerr << "Mod" << endl;
  }
  else if (kind_ == Kind::Draw) {
    cerr << "Draw" << endl;
  }
  else if (kind_ == Kind::Draws) {
    cerr << "MultDraw" << endl;
  }
  else {
    cerr << "Unknown" << endl;
  }
}

NodePtr parse_(const vector<string>& tokens, int& i) {
  if ((int)tokens.size() == i) return nullptr;

  const auto& token = tokens[i];
  ++i;

  if (token.empty()) return parse_(tokens, i);
  if (isdigit(token[0]) || token[0] == '+' || token[0] == '-') {
    auto node = make_shared<Node>();
    node->kind_ = Kind::Number;
    node->name_ = token;
    return node;
  }
  else if (token == "ap") {
    auto node = make_shared<Node>();
    node->kind_ = Kind::App;
    node->fun_ = parse_(tokens, i);
    node->arg_ = parse_(tokens, i);
    return node;
  }
  else {
    auto node = make_shared<Node>();
    node->kind_ = Kind::Fun;
    node->name_ = token;
    return node;
  }

  return nullptr;
}
NodePtr parse(const vector<string>& tokens) {
  int i = 0;
  return parse_(tokens, i);
}

NodePtr tolist(const vector<NodePtr>& nodes, size_t i = 0) {
  if (nodes.size() == i) return make_shared<NilNode>();

  auto ap1 = Node::make(Kind::App);
  auto ap2 = Node::make(Kind::App);
  ap1->fun_ = ap2;

  ap2->fun_ = Node::make(Kind::Fun);
  ap2->fun_->name_ = "cons";
  ap2->arg_ = nodes[i];

  ap1->arg_ = tolist(nodes, i+1);
  return ap1;
}

/*
  implement list (in galaxy.txt)
    * add
    * ap
    * b
    * c
    * car
    * cdr
    * cons
    * div
    * eq
    * i
    * isnil
    * lt
    * mul
    * neg
    * nil
    * s
    * t
  to interact ...
    * if0
    * mod
    * dem
    * draw
    * multipledraw
    * send
    interact 
*/

map<string, NodePtr> gTable;

ObjectPtr Node::eval() {
  if (val_ != nullptr) return val_;
  auto make_app = [](NodePtr a, NodePtr b) {
    NodePtr node = make_shared<Node>();
    node->kind_ = Kind::App;
    node->fun_ = a;
    node->arg_ = b;
    // node->name_ = a->name_ + "{" + b->name_ + "}";
    return node;
  };

  if (kind_ == Kind::Number) {
    val_ = Object::make(stol(name_));
  }
  else if (kind_ == Kind::App) {
    auto fun = fun_->eval();
    val_ = fun->call(arg_)->eval();
  }
  else if (kind_ == Kind::Fun) {
    // #5
    if (name_ == "inc") {
      val_ = Func1::make([&](NodePtr arg){
        return Node::make(Kind::Number, Object::make(arg->eval()->val() + 1));
      }, "inc");
    }
    // #7
    else if (name_ == "add") {
      val_ = Func2::make([=](NodePtr arg1, NodePtr arg2){
        return Node::make(Kind::Number,
          Object::make(arg1->eval()->val() + arg2->eval()->val()));
      }, "add");
    }
    // #9
    else if (name_ == "mul") {
      val_ = Func2::make([=](NodePtr arg1, NodePtr arg2){
        return Node::make(Kind::Number,
          Object::make(arg1->eval()->val() * arg2->eval()->val()));
      }, "mul");
    }
    // #10
    else if (name_ == "div") {
      val_ = Func2::make([=](NodePtr arg1, NodePtr arg2){
        return Node::make(Kind::Number,
          Object::make(arg1->eval()->val() / arg2->eval()->val()));
      }, "div");
    }
    // #11
    else if (name_ == "eq") {
      val_ = Func2::make([=](NodePtr arg1, NodePtr arg2){
        return
          arg1->eval()->val() == arg2->eval()->val()
          ? TrueNode() : FalseNode();
      }, "eq");
    }
    // #12
    else if (name_ == "lt") {
      val_ = Func2::make([=](NodePtr arg1, NodePtr arg2){
        return arg1->eval()->val() < arg2->eval()->val() ? TrueNode() : FalseNode();
      }, "lt");
    }
    else if (name_ == "mod") {
      val_ = Func1::make([=](NodePtr arg1){
        return arg1->eval()->mod();
      }, "mod");
    }
    else if (name_ == "dem") {
      val_ = Func1::make([=](NodePtr arg1){
        return arg1->eval()->dem();
      }, "dem");
    }
    // #16
    else if (name_ == "neg") {
      val_ = Func1::make([=](NodePtr arg){
        return Node::make(Kind::Number, Object::make(- arg->eval()->val()));
      }, "neg");
    }
    // #18
    else if (name_ == "s") {
      val_ = Func3::make([=](NodePtr arg1, NodePtr arg2, NodePtr arg3){
        auto tmp = make_app(arg2, arg3);
        return arg1->eval()->call(arg3)->eval()->call(tmp);
      }, "s");
    }
    // #19
    else if (name_ == "c") {
      val_ = Func3::make([=](NodePtr arg1, NodePtr arg2, NodePtr arg3){
        return arg1->eval()->call(arg3)->eval()->call(arg2);
      }, "c");
    }
    // #20
    else if (name_ == "b") {
      val_ = Func3::make([=](NodePtr arg1, NodePtr arg2, NodePtr arg3){
        auto tmp = make_app(arg2, arg3);
        return arg1->eval()->call(tmp);
      }, "b");
    }
    // #21
    else if (name_ == "t") {
      val_ = True();
    }
    else if (name_ == "f") {
      val_ = False();
    }
    // #24
    else if (name_ == "i") {
      val_ = Func1::make([=](NodePtr arg1){
        return arg1;
      }, "i");
    }
    // #25
    else if (name_ == "cons") {
      val_ = Func3::make([=](NodePtr arg1, NodePtr arg2, NodePtr arg3){
        auto tmp = make_app(arg3, arg1);
        auto tmp2 = make_app(tmp, arg2);
        return tmp2;

        // return arg3->eval()->call(arg1)->eval()->call(arg2);
      }, "cons");
    }
    // #26
    else if (name_ == "car") {
      val_ = Func1::make([=](NodePtr arg1){
        auto tmp = make_app(arg1, TrueNode());
        return tmp;
        // return arg1->eval()->call(TrueNode());
      }, "car");
    }
    // #27
    else if (name_ == "cdr") {
      val_ = Func1::make([=](NodePtr arg1){
        auto tmp = make_app(arg1, FalseNode());
        return tmp;
        // return arg1->eval()->call(FalseNode());
      }, "cdr");
    }
    // #28
    else if (name_ == "nil") {
      val_ = make_shared<Nil>();
    }
    // #29
    else if (name_ == "isnil") {
      val_ = Func1::make([=](NodePtr arg1){
        return arg1->eval()->is_nil() ? TrueNode() : FalseNode();
      }, "isnil");
    }
    // #32
    else if (name_ == "draw") {
      val_ = Func1::make([=](NodePtr arg1){
        return draw(arg1);
      }, "draw");
    }
    // #34
    else if (name_ == "multipledraw") {
      val_ = Func1::make([=](NodePtr arg1){
        return drawlist(arg1);
      }, "multipledraw");
    }
    // #36
    else if (name_ == "send") {
      val_ = Func1::make([=](NodePtr arg1){
        return arg1->eval()->send();
      }, "send");
    }
    // #37
    else if (name_ == "if0") {
      val_ = Func3::make([=](NodePtr arg1, NodePtr arg2, NodePtr arg3){
        return arg1->eval()->val() == 0 ? arg2 : arg3;
      }, "if0");
    }
    else if (name_ == "interact") {
      val_ = Func3::make([=](NodePtr arg1, NodePtr arg2, NodePtr arg3){
        while(true) {
          auto next = arg1->eval()->call(arg2)->eval()->call(arg3)->eval();
          auto flag = next->call(TrueNode());
          next = next->call(FalseNode())->eval();
          auto newState = next->call(TrueNode());
          // auto newState = next->call(TrueNode())->eval()->mod()->eval()->dem();
          next = next->call(FalseNode())->eval();
          auto data = next->call(TrueNode());

          if (flag->eval()->val() == 0) {
            auto fst = newState;

            auto draw = Node::make(Kind::Fun);
            draw->name_ = "multipledraw";
            // auto snd = make_app(draw, data)->eval();
            auto snd = draw->eval()->call(data);

            return tolist({fst, snd});
          }

          arg2 = newState;
          arg3 = data->eval()->send();
        }
      }, "interact");
    }
    else if (gTable.count(name_)) {
      auto child = gTable[name_];
      val_ = child->eval();
    }
    else {
      cerr << "unknown function: " << name_ << endl;
    }
  }
  if (val_ == nullptr) {
    throw runtime_error("cannot evaluate");
  }

  return val_;
}


vector<string> tokenize(const string& line) {
  vector<string> tokens;
  stringstream ss(line);
  while(ss) {
    string token;
    ss >> token;
    if(!ss) break;
    tokens.emplace_back(token);
  }
  return tokens;
}

void create_depends(map<string, set<string>>& G, map<string, NodePtr>& table) {
  function<void(set<string>& s, NodePtr node)> search = [&](set<string>& s, NodePtr node) {
    if (node->kind_ == Kind::Fun) {
      s.insert(node->name_);
    }
    else if (node->kind_ == Kind::App) {
      search(s, node->fun_);
      search(s, node->arg_);
    }
  };

  for(auto &entry : table){
    search(G[entry.first], entry.second);
  }
}

bool topo_sort(map<string, set<string>>& G, vector<string>& order) {
  map<string,int> color;
  function<bool(const string&)> dfs = [&](const string& u) {
    color[u] = 1;
    for(auto & e : G[u]) {
      if (color[e] == 2) continue;
      if (color[e] == 1) { cerr << u << "->" << e << endl; return false; }
      dfs(e);
    }
    color[u] = 2;

    order.push_back(u);
    return true;
  };

  for(auto & u : G) {
    if (color[u.first] == 0) {
      if(!dfs(u.first)) return false;
    }
  }

  reverse(begin(order), end(order));
  return true;
}

int main(int argc, char* argv[]) {
	curl_global_init(CURL_GLOBAL_ALL);

  if (argc < 2) {
    cerr << "usage: " << argv[0] << " galaxy.txt" << endl;
    return 1;
  }

  {
    ifstream ifs(argv[1]);
    if (ifs.fail()) {
      cerr << "failed to open " << argv[1] << endl;
      return 1;
    }

    string line;
    while(getline(ifs, line)) {
      stringstream ss(line);
      string var, eq;
      ss >> var;
      ss >> eq;

      if (eq != "=") {
        cerr << "not assignment" << endl;
        continue;
      }
      string decl;
      getline(ss, decl);

      auto node = parse(tokenize(decl));
      gTable[var] = node;
    }
  }

  map<string, ObjectPtr> objects;
  objects["galaxy"] = gTable["galaxy"]->eval();

#if 0
  while(1) {
    cout << "> " << flush;
    string line;
    getline(cin, line);

    if (line.find("=") == string::npos) {
      auto node = parse(tokenize(line));
      node->eval()->dump();
      cerr<<endl;
    }
    else {
      stringstream ss(line);
      string var, eq;
      ss >> var;
      ss >> eq;

      string decl;
      getline(ss, decl);

      auto node = parse(tokenize(decl));
      gTable[var] = node;
      gTable[var]->eval()->dump();
      cerr<<endl;
    }
  }
#else
  auto state = make_shared<NilNode>();
  gTable["_state"] = state;

  while(1) {
    cout << "> " << flush;

    long x, y;
    cin >> x >> y;

    string cmd = "ap ap ap interact galaxy _state ap ap cons "
      + to_string(x) + " " + to_string(y);

    gTable["_internal"] = parse(tokenize(cmd));

    string get_state_cmd = "ap car _internal";
    string get_drawlist_cmd = "ap car ap cdr _internal";

    auto windows = parse(tokenize(get_drawlist_cmd))->eval();
    windows->dump();
    cerr << endl;

    auto next = parse(tokenize(get_state_cmd));
    cerr << "_state: ";
    next->eval()->dump();
    cerr << endl;

    gTable["_state"] = next;
  }
#endif

  return 0;
}