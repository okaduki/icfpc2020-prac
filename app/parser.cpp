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

  virtual ObjectPtr call(ObjectPtr arg) {
    throw runtime_error("object is not callable");
  }

  void dump() const {
    if (is_val_) cerr << "val: " << val_ << endl;
    else if (is_nil_) cerr << "nil" << endl;
    else cerr << "func " << name_ << endl;
  }
};

struct Nil : public Object {
  Nil() : Object() { is_nil_ = true; }
};

struct Func1 : public Object {
  using Type = function<ObjectPtr(ObjectPtr)>;
  Type f_;
  Func1 (Type f, const string& name = "") : Object(), f_(f) { name_ = name; }
  virtual ObjectPtr call(ObjectPtr obj) override {
    return f_(obj);
  }
};

struct Func2 : public Object {
  using Type = function<ObjectPtr(ObjectPtr,ObjectPtr)>;
  Type f_;
  Func2 (Type f, const string& name = "") : Object(), f_(f) { name_ = name; }
  virtual ObjectPtr call(ObjectPtr arg1) override {
    auto f = f_;
    return make_shared<Func1>(
      [f,arg1](ObjectPtr arg2) {
        return f(arg1, arg2);
      }
      , name_ + "(" + arg1->name() + ")"
    );
  }
};

struct Func3 : public Object {
  using Type = function<ObjectPtr(ObjectPtr,ObjectPtr,ObjectPtr)>;
  Type f_;
  Func3 (Type f, const string& name = "") : Object(), f_(f) { name_ = name; }
  virtual ObjectPtr call(ObjectPtr arg1) override {
    auto f = f_;
    return make_shared<Func2>(
      [f,arg1](ObjectPtr arg2, ObjectPtr arg3) {
        return f(arg1, arg2, arg3);
      }
      , name_ + "(" + arg1->name() + ")"
    );
  }
};

ObjectPtr True () {
  return make_shared<Func2> (
    [&](ObjectPtr arg1, ObjectPtr arg2) {
      return arg1;
    }
    , "true"
  );
}

ObjectPtr False () {
  return make_shared<Func2> (
    [&](ObjectPtr arg1, ObjectPtr arg2) {
      return arg2;
    }
  );
}


struct Node {
  Node();
  // NodePtr apply(NodePtr arg);
  void dump(int indent = 0) const;

  Kind kind_;
  string name_;
  NodePtr fun_;
  NodePtr arg_;
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

ObjectPtr eval(NodePtr node) {
  auto make = [](auto val) { return make_shared<Object>(val); };
  auto make_f1 = [](auto val, const string name = "") { return make_shared<Func1>(val, name); };
  auto make_f2 = [](auto val, const string name = "") { return make_shared<Func2>(val, name); };
  auto make_f3 = [](auto val, const string name = "") { return make_shared<Func3>(val, name); };

  if (node->kind_ == Kind::Number) {
    return make(stol(node->name_));
  }
  else if (node->kind_ == Kind::App) {
    auto fun = eval(node->fun_);
    auto arg = eval(node->arg_);

    return fun->call(arg);
  }
  else if (node->kind_ == Kind::Fun) {
    // #4
    if (node->name_ == "eq") {
      return make_f2([=](ObjectPtr arg1, ObjectPtr arg2){
        return arg1->val() == arg2->val() ? True() : False();
      });
    }
    // #5
    else if (node->name_ == "inc") {
      return make_f1([&](ObjectPtr arg){
        return make(arg->val() + 1);
      }, "inc");
    }
    // #7
    else if (node->name_ == "add") {
      return make_f2([=](ObjectPtr arg1, ObjectPtr arg2){
        return make(arg1->val() + arg2->val());
      });
    }
    // #8
    else if (node->name_ == "mul") {
      return make_f2([=](ObjectPtr arg1, ObjectPtr arg2){
        return make(arg1->val() * arg2->val());
      });
    }
    // #9
    else if (node->name_ == "div") {
      return make_f2([=](ObjectPtr arg1, ObjectPtr arg2){
        return make(arg1->val() / arg2->val());
      });
    }
    // #10
    else if (node->name_ == "lt") {
      return make_f2([=](ObjectPtr arg1, ObjectPtr arg2){
        return arg1->val() < arg2->val() ? True() : False();
      });
    }
    // #16
    else if (node->name_ == "neg") {
      return make_f1([=](ObjectPtr arg){
        return make(-arg->val());
      });
    }
    // #18
    else if (node->name_ == "s") {
      return make_f3([=](ObjectPtr arg1, ObjectPtr arg2, ObjectPtr arg3){
        auto tmp = arg2->call(arg3);
        return arg1->call(arg3)->call(tmp);
      });
    }
    // #19
    else if (node->name_ == "c") {
      return make_f3([=](ObjectPtr arg1, ObjectPtr arg2, ObjectPtr arg3){
        auto tmp = arg1->call(arg3);
        return tmp->call(arg2);
      });
    }
    // #20
    else if (node->name_ == "b") {
      return make_f3([=](ObjectPtr arg1, ObjectPtr arg2, ObjectPtr arg3){
        auto tmp = arg2->call(arg3);
        return arg1->call(tmp);
      }, "b");
    }
    // #21
    else if (node->name_ == "t") {
      return True();
    }
    // #24
    else if (node->name_ == "i") {
      return make_f1([=](ObjectPtr arg1){
        return arg1;
      });
    }
    // #25
    else if (node->name_ == "cons") {
      return make_f3([=](ObjectPtr arg1, ObjectPtr arg2, ObjectPtr arg3){
        return arg3->call(arg1)->call(arg2);
      }
      , "cons");
    }
    // #26
    else if (node->name_ == "car") {
      return make_f1([=](ObjectPtr arg1){
        cerr << "calling lambda f1 in car "
          << arg1->name() << endl;
        return arg1->call(True());
      }
      , "car");
    }
    // #27
    else if (node->name_ == "cdr") {
      return make_f1([=](ObjectPtr arg1){
        return arg1->call(False());
      }
      , "cdr");
    }
    // #28
    else if (node->name_ == "nil") {
      return make_shared<Nil>();
    }
    // #29
    else if (node->name_ == "isnil") {
      return make_f1([=](ObjectPtr arg1){
        return arg1->is_nil() ? True() : False();
      });
    }
    else {
      cerr << "unknown function: " << node->name_ << endl;
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

int main() {
  while(1){
    string stmt;
    getline(cin, stmt);

    vector<string> tokens;
    tokenize(stmt, tokens);

    int i = 0;
    auto node = parse(tokens, i);
    // node->dump();
    eval(node)->dump();
  }

  return 0;
}