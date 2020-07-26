#include <bits/stdc++.h>
#include <curl/curl.h>

using namespace std;

enum class Kind {
  Number,
  Fun,
  App,
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
  Object(long val) : val_(val), is_val_ (true), is_mod_(false) {}
  // Object(FuncPtr func) : fun_(func), val_(-1), is_val_ (false) {}

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

  virtual ObjectPtr call(NodePtr arg) {
    throw runtime_error("object is not callable");
  }

  ObjectPtr send();

  virtual void dump() const {
    if (is_val_) cerr << "val: " << val_ << endl;
    else if (is_nil_) cerr << "nil" << endl;
    else if (is_mod_) cerr << "[" << mod_ << "]" << endl;
    else cerr << "func " << name_ << endl;
  }

  ObjectPtr mod();
  ObjectPtr dem();
};

struct Node {
  Node();
  // NodePtr apply(NodePtr arg);
  void dump(int indent = 0) const;
  ObjectPtr eval();

  Kind kind_;
  string name_;
  NodePtr fun_;
  NodePtr arg_;
};

struct Nil : public Object {
  Nil() : Object() { is_nil_ = true; }
};

struct Func1 : public Object {
  using Type = function<ObjectPtr(NodePtr)>;
  Type f_;
  Func1 (Type f) : Object(), f_(f) {}
  virtual ObjectPtr call(NodePtr obj) override {
    return f_(obj);
  }
};

struct Func2 : public Object {
  using Type = function<ObjectPtr(NodePtr,NodePtr)>;
  Type f_;
  Func2 (Type f) : Object(), f_(f) { }
  virtual ObjectPtr call(NodePtr arg1) override {
    auto f = f_;
    return make_shared<Func1>(
      [f,arg1](NodePtr arg2) {
        return f(arg1, arg2);
      }
    );
  }
};

struct Func3 : public Object {
  using Type = function<ObjectPtr(NodePtr,NodePtr,NodePtr)>;
  Type f_;
  Func3 (Type f) : Object(), f_(f) { }
  virtual ObjectPtr call(NodePtr arg1) override {
    auto f = f_;
    return make_shared<Func2>(
      [f,arg1](NodePtr arg2, NodePtr arg3) {
        return f(arg1, arg2, arg3);
      }
    );
  }
};

ObjectPtr True () {
  return make_shared<Func2> (
    [&](NodePtr arg1, NodePtr arg2) {
      return arg1->eval();
    }
  );
}

NodePtr TrueNode() {
  NodePtr node = make_shared<Node>();
  node->kind_ = Kind::Fun;
  node->name_ = "t";
  return node;
};

ObjectPtr False () {
  return make_shared<Func2> (
    [&](NodePtr arg1, NodePtr arg2) {
      return arg2->eval();
    }
  );
}

NodePtr FalseNode() {
  NodePtr node = make_shared<Node>();
  node->kind_ = Kind::Fun;
  node->name_ = "f";
  return node;
};


ObjectPtr Object::mod() {
  if (is_val_) {
    ObjectPtr res = make_shared<Object>();
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
    return res;
  }
  else {
    string mod;
    if (is_nil()) {
      mod += "00";
    }
    else {
      auto hd = this->call(TrueNode());
      auto tl = this->call(FalseNode());

      mod += "11";
      mod += hd->mod()->mod_;
      mod += tl->mod()->mod_;
    }

    ObjectPtr res = make_shared<Object>();
    res->mod_ = mod;
    res->is_mod_ = true;
    return res;
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
    auto res = make_shared<Node>();
    res->kind_ = Kind::Number;
    res->name_ = to_string(val);
    return res;
  }
  else { // list
    ix += 2;
    if (sig == "00") {
      auto res = make_shared<Node>();
      res->kind_ = Kind::Fun;
      res->name_ = "nil";
      return res;
    }

    auto hd = dem_str(mod, ix);
    auto tl = dem_str(mod, ix);

    auto cons = make_shared<Node>();
    cons->kind_ = Kind::Fun;
    cons->name_ = "cons";

    auto res_fun = make_shared<Node>();
    res_fun->kind_ = Kind::App;
    res_fun->fun_ = cons;
    res_fun->arg_ = hd;

    auto res = make_shared<Node>();
    res->kind_ = Kind::App;
    res->fun_ = res_fun;
    res->arg_ = tl;

    return res;
  }
}

ObjectPtr Object::dem() {
  int i = 0;
  auto node = dem_str(mod_, i);
  return node->eval();
}

struct Draw : public Object {
  vector<pair<long,long>> points;
  Draw() : Object() {}

  void dump() const override {
    cerr << "(";
    for(const auto& p : points) {
      cerr << "(" << p.first << " " << p.second << ")";
    }
    cerr << ")" << endl;
  }
};

struct DrawList : public Object {
  vector<vector<pair<long,long>>> pointss;
  DrawList() : Object() {}

  void dump() const override {
    cerr << "(";
    for(const auto& points : pointss) {
      cerr << "(";
      for(const auto & p : points)
        cerr << "(" << p.first << " " << p.second << ")";
      cerr << ") ";
    }
    cerr << ")" << endl;
  }
};

shared_ptr<Draw> draw(ObjectPtr obj) {
  auto res = make_shared<Draw>();
  while(true){
    if (obj->is_nil()) break;

    auto point = obj->call(TrueNode());
    auto x = point->call(TrueNode())->val();
    auto y = point->call(FalseNode())->call(TrueNode())->val();

    res->points.emplace_back(x, y);
    obj = obj->call(FalseNode());
  }

  return res;
}

shared_ptr<Draw> draw(NodePtr n) {
  return draw(n->eval());
}

shared_ptr<DrawList> drawlist(ObjectPtr obj) {
  auto res = make_shared<DrawList>();
  while(true){
    if (obj->is_nil()) break;

    auto points = obj->call(TrueNode());
    auto tmp = draw(points);

    res->pointss.emplace_back(tmp->points);
    obj = obj->call(FalseNode());
  }

  return res;
}

shared_ptr<DrawList> drawlist(NodePtr n) {
  return drawlist(n->eval());
}

size_t send_callback(char *ptr, size_t size, size_t nmemb, string *stream) {
  int dataLength = size * nmemb;
  stream->append(ptr, dataLength);
  return dataLength;
}

string send(const string& str) {
  CURL *curl = curl_easy_init();

  string resp;
  curl_easy_setopt(curl, CURLOPT_URL, "https://icfpc2020-api.testkontur.ru/aliens/send?apiKey=b0a3d915b8d742a39897ab4dab931721");
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

ObjectPtr Object::send() {
  auto obj_mod = this->mod();
  string str = obj_mod->mod_val();
  string resp = ::send(str);

  obj_mod->mod_ = resp;
  return obj_mod->dem();
}

Node::Node(){}

void Node::dump(int indent) const {
  cerr << string(indent, ' ');
  if (kind_ == Kind::Number) {
    cerr << "Num " << name_ << std::endl;
  }
  else if (kind_ == Kind::Fun) {
    cerr << "Fun " << name_ << std::endl;
  }
  else if (kind_ == Kind::App) {
    cerr << "App" << std::endl;
    fun_->dump(indent + 2);
    arg_->dump(indent + 2);
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
  auto make = [](auto val) { return make_shared<Object>(val); };
  auto make_f1 = [](auto val) { return make_shared<Func1>(val); };
  auto make_f2 = [](auto val) { return make_shared<Func2>(val); };
  auto make_f3 = [](auto val) { return make_shared<Func3>(val); };
  auto make_tmp = [](NodePtr a, NodePtr b) {
    NodePtr node = make_shared<Node>();
    node->kind_ = Kind::App;
    node->fun_ = a;
    node->arg_ = b;
    return node;
  };

  if (kind_ == Kind::Number) {
    return make(stol(name_));
  }
  else if (kind_ == Kind::App) {
    auto fun = fun_->eval();

    return fun->call(arg_);
  }
  else if (kind_ == Kind::Fun) {
    // #5
    if (name_ == "inc") {
      return make_f1([&](NodePtr arg){
        return make(arg->eval()->val() + 1);
      });
    }
    // #7
    else if (name_ == "add") {
      return make_f2([=](NodePtr arg1, NodePtr arg2){
        return make(arg1->eval()->val() + arg2->eval()->val());
      });
    }
    // #9
    else if (name_ == "mul") {
      return make_f2([=](NodePtr arg1, NodePtr arg2){
        return make(arg1->eval()->val() * arg2->eval()->val());
      });
    }
    // #10
    else if (name_ == "div") {
      return make_f2([=](NodePtr arg1, NodePtr arg2){
        return make(arg1->eval()->val() / arg2->eval()->val());
      });
    }
    // #11
    else if (name_ == "eq") {
      return make_f2([=](NodePtr arg1, NodePtr arg2){
        return arg1->eval()->val() == arg2->eval()->val() ? True() : False();
      });
    }
    // #12
    else if (name_ == "lt") {
      return make_f2([=](NodePtr arg1, NodePtr arg2){
        return arg1->eval()->val() < arg2->eval()->val() ? True() : False();
      });
    }
    else if (name_ == "mod") {
      return make_f1([=](NodePtr arg1){
        return arg1->eval()->mod();
      });
    }
    else if (name_ == "dem") {
      return make_f1([=](NodePtr arg1){
        return arg1->eval()->dem();
      });
    }
    // #16
    else if (name_ == "neg") {
      return make_f1([=](NodePtr arg){
        return make(- arg->eval()->val());
      });
    }
    // #18
    else if (name_ == "s") {
      return make_f3([=](NodePtr arg1, NodePtr arg2, NodePtr arg3){
        auto tmp = make_tmp(arg2, arg3);
        return arg1->eval()->call(arg3)->call(tmp);
      });
    }
    // #19
    else if (name_ == "c") {
      return make_f3([=](NodePtr arg1, NodePtr arg2, NodePtr arg3){
        auto tmp = make_tmp(arg1, arg3);
        return tmp->eval()->call(arg2);
      });
    }
    // #20
    else if (name_ == "b") {
      return make_f3([=](NodePtr arg1, NodePtr arg2, NodePtr arg3){
        auto tmp = make_tmp(arg2, arg3);
        return arg1->eval()->call(tmp);
      });
    }
    // #21
    else if (name_ == "t") {
      return True();
    }
    else if (name_ == "f") {
      return False();
    }
    // #24
    else if (name_ == "i") {
      return make_f1([=](NodePtr arg1){
        return arg1->eval();
      });
    }
    // #25
    else if (name_ == "cons") {
      return make_f3([=](NodePtr arg1, NodePtr arg2, NodePtr arg3){
        return arg3->eval()->call(arg1)->call(arg2);
      });
    }
    // #26
    else if (name_ == "car") {
      return make_f1([=](NodePtr arg1){
        return arg1->eval()->call(TrueNode());
      });
    }
    // #27
    else if (name_ == "cdr") {
      return make_f1([=](NodePtr arg1){
        return arg1->eval()->call(FalseNode());
      });
    }
    // #28
    else if (name_ == "nil") {
      return make_shared<Nil>();
    }
    // #29
    else if (name_ == "isnil") {
      return make_f1([=](NodePtr arg1){
        return arg1->eval()->is_nil() ? True() : False();
      });
    }
    // #32
    else if (name_ == "draw") {
      return make_f1([=](NodePtr arg1){
        return draw(arg1);
      });
    }
    // #34
    else if (name_ == "multipledraw") {
      return make_f1([=](NodePtr arg1){
        return drawlist(arg1);
      });
    }
    // #36
    else if (name_ == "send") {
      return make_f1([=](NodePtr arg1){
        return arg1->eval()->send();
      });
    }
    // #37
    else if (name_ == "if0") {
      return make_f3([=](NodePtr arg1, NodePtr arg2, NodePtr arg3){
        return arg1->eval()->val() == 0 ? arg2->eval() : arg3->eval();
      });
    }
    else if (gTable.count(name_)) {
      auto child = gTable[name_];
      return child->eval();
    }
    else {
      cerr << "unknown function: " << name_ << endl;
    }
  }

  return nullptr;
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

  while(1) {
    cout << "> " << flush;
    string line;
    getline(cin, line);

    auto node = parse(tokenize(line));
    node->eval()->dump();
  }

  return 0;
}