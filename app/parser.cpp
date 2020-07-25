#include <bits/stdc++.h>

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
  bool is_val_;
  bool is_nil_;
  string name_;

public:
  Object() : val_(0), is_val_(false), is_nil_(false) {}
  Object(long val) : val_(val), is_val_ (true) {}
  // Object(FuncPtr func) : fun_(func), val_(-1), is_val_ (false) {}

  long val() const {
    if (!is_val_) {
      throw runtime_error("not value");
    }
    return val_;
  }
  string name() const { return is_val_ ? "val:" + to_string(val_) : is_nil_ ? "nil" : name_; }

  bool is_val() const { return is_val_; }
  bool is_nil() const { return is_nil_; }

  virtual ObjectPtr call(NodePtr arg) {
    throw runtime_error("object is not callable");
  }

  void dump() const {
    if (is_val_) cerr << "val: " << val_ << endl;
    else if (is_nil_) cerr << "nil" << endl;
    else cerr << "func " << name_ << endl;
  }
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


NodePtr parse(const vector<string>& tokens, int& i) {
  if ((int)tokens.size() == i) return nullptr;

  const auto& token = tokens[i];
  ++i;

  if (token.empty()) return parse(tokens, i);
  if (isdigit(token[0]) || token[0] == '+' || token[0] == '-') {
    auto node = make_shared<Node>();
    node->kind_ = Kind::Number;
    node->name_ = token;
    return node;
  }
  else if (token == "ap") {
    auto node = make_shared<Node>();
    node->kind_ = Kind::App;
    node->fun_ = parse(tokens, i);
    node->arg_ = parse(tokens, i);
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
  cerr << "eval " << name_ << endl;

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


void tokenize(const string& line, vector<string>& tokens) {
  stringstream ss(line);
  while(ss) {
    string token;
    ss >> token;
    if(!ss) break;
    tokens.emplace_back(token);
  }
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

      vector<string> tokens;
      tokenize(decl, tokens);

      int i = 0;
      auto node = parse(tokens, i);
      gTable[var] = node;
    }
  }

  map<string, ObjectPtr> objects;
  objects["galaxy"] = gTable["galaxy"]->eval();

  return 0;
}