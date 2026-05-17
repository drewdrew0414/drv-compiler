#include "codegen.h"
#include <cassert>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace drv {

Codegen::Codegen(CodegenOptions opts) : opts_(std::move(opts)) {}

// ─────────────────────────────────────────────────────────────────────────────
// Type mapping: drv → C++
// ─────────────────────────────────────────────────────────────────────────────
std::string Codegen::mapBuiltinType(const std::string& name) {
    if (name=="int")     return "int32_t";
    if (name=="long")    return "int64_t";
    if (name=="float")   return "float";
    if (name=="double")  return "double";
    if (name=="char")    return "char";
    if (name=="String")  return "std::string";
    if (name=="boolean") return "bool";
    if (name=="void")    return "void";
    if (name=="var")     return "auto";
    if (name=="tensor")  return "std::vector<double>"; // dynamic tensor fallback
    if (name=="list")    return "std::vector";
    if (name=="Map")     return "std::unordered_map";
    if (name=="Own")     return "std::unique_ptr";
    if (name=="Ref")     return "std::shared_ptr";
    if (name=="Borrow")  return "const ";
    if (name=="Option")  return "std::optional";
    if (name=="Result")  return "DrvResult";
    if (name=="Self")    return "auto";
    return name; // user-defined types pass through
}

std::string Codegen::mapType(const TypeRef& tr) {
    std::string base = mapBuiltinType(tr.name);

    // generic args
    if (!tr.args.empty()) {
        base += "<";
        for (size_t i=0; i<tr.args.size(); ++i) {
            if (i) base += ", ";
            base += mapType(tr.args[i]);
        }
        base += ">";
    }

    if (tr.is_array) {
        // int[] → std::vector<int32_t>
        if (tr.name=="int")     return "std::vector<int32_t>";
        if (tr.name=="long")    return "std::vector<int64_t>";
        if (tr.name=="float")   return "std::vector<float>";
        if (tr.name=="double")  return "std::vector<double>";
        if (tr.name=="char")    return "std::string"; // char[] → string
        if (tr.name=="String")  return "std::vector<std::string>";
        return "std::vector<" + base + ">";
    }

    if (tr.is_mut) base = base; // mut is just a note, not a C++ keyword

    return base;
}

// ─────────────────────────────────────────────────────────────────────────────
// Built-in function mapping
// ─────────────────────────────────────────────────────────────────────────────
static std::string argsStr(const ExprList& args, Codegen* cg);

std::string Codegen::mapBuiltinCall(const std::string& name, const ExprList& args) {
    auto a = [&](int i) -> std::string { return i < (int)args.size() ? emitExpr(*args[i]) : ""; };
    std::string as = argsStr(args, this);

    if (name=="print")  return "__drv_print(" + as + ")";
    if (name=="input")  return "__drv_input(" + as + ")";
    if (name=="length") return "__drv_length(" + a(0) + ")";
    if (name=="split")  return "__drv_split(" + a(0) + ", " + a(1) + ")";
    if (name=="abs")    return "std::abs(" + a(0) + ")";
    if (name=="max")    return "std::max(" + a(0) + ", " + a(1) + ")";
    if (name=="min")    return "std::min(" + a(0) + ", " + a(1) + ")";
    if (name=="sqrt")   return "std::sqrt(" + a(0) + ")";
    if (name=="floor")  return "std::floor(" + a(0) + ")";
    if (name=="ceil")   return "std::ceil(" + a(0) + ")";
    if (name=="round")  return "std::round(" + a(0) + ")";
    if (name=="sin")    return "std::sin(" + a(0) + ")";
    if (name=="cos")    return "std::cos(" + a(0) + ")";
    if (name=="tan")    return "std::tan(" + a(0) + ")";
    if (name=="fma")    return "std::fma(" + a(0) + ", " + a(1) + ", " + a(2) + ")";
    if (name=="likely") return "[[likely]] " + a(0);
    if (name=="unlikely") return "[[unlikely]] " + a(0);
    if (name=="sum")    return "__drv_sum(" + a(0) + ")";
    if (name=="mean")   return "__drv_mean(" + a(0) + ")";
    if (name=="dot")    return "__drv_dot(" + a(0) + ", " + a(1) + ")";
    if (name=="norm")   return "__drv_norm(" + a(0) + ")";
    if (name=="sort")   return "__drv_sort(" + a(0) + ")";
    if (name=="reverse") return "__drv_reverse(" + a(0) + ")";
    if (name=="range")  return args.size()==1 ? "__drv_range(0," + a(0) + ")" : "__drv_range(" + a(0) + "," + a(1) + ")";
    if (name=="toUpper") return "__drv_toUpper(" + a(0) + ")";
    if (name=="toLower") return "__drv_toLower(" + a(0) + ")";
    if (name=="trim")   return "__drv_trim(" + a(0) + ")";
    if (name=="replace") return "__drv_replace(" + a(0) + ", " + a(1) + ", " + a(2) + ")";
    if (name=="assert_that") return "__drv_assert_that(" + a(0) + ", " + a(1) + ")";
    if (name=="compile_eval") return "/* compile_eval */ (" + a(0) + ")";
    if (name=="__array_init") {
        if (args.empty()) return "{}";
        return "std::vector{" + as + "}";  // CTAD deduces element type
    }
    if (name=="__map_init")   return "{}"; // empty map literal

    return name + "(" + as + ")";
}

std::string Codegen::mapBuiltinNamespace(const std::string& ns, const std::string& fn,
                                         const ExprList& args) {
    auto a = [&](int i) -> std::string { return i < (int)args.size() ? emitExpr(*args[i]) : ""; };
    std::string as = argsStr(args, this);

    // str.*
    if (ns=="str") {
        if (fn=="from_int")    return "std::to_string(" + a(0) + ")";
        if (fn=="from_long")   return "std::to_string(" + a(0) + ")";
        if (fn=="from_float")  return "std::to_string(" + a(0) + ")";
        if (fn=="to_int")      return "std::stoi(" + a(0) + ")";
        if (fn=="to_float")    return "std::stod(" + a(0) + ")";
        if (fn=="contains")    return "(" + a(0) + ".find(" + a(1) + ")!=std::string::npos)";
        if (fn=="starts_with") return "(" + a(0) + ".starts_with(" + a(1) + "))";
        if (fn=="ends_with")   return "(" + a(0) + ".ends_with(" + a(1) + "))";
        if (fn=="substr")      return a(0) + ".substr(" + a(1) + ", " + a(2) + ")";
        if (fn=="reverse")     return "__drv_str_reverse(" + a(0) + ")";
        if (fn=="repeat")      return "__drv_str_repeat(" + a(0) + ", " + a(1) + ")";
        if (fn=="pad_left")    return "__drv_str_pad_left(" + a(0) + ", " + a(1) + ")";
        if (fn=="pad_right")   return "__drv_str_pad_right(" + a(0) + ", " + a(1) + ")";
        if (fn=="is_empty")    return a(0) + ".empty()";
        if (fn=="join")        return "__drv_str_join(" + a(0) + ", " + a(1) + ")";
        if (fn=="index_of")    return "static_cast<int64_t>(" + a(0) + ".find(" + a(1) + "))";
        if (fn=="char_at")     return "std::string(1," + a(0) + "[" + a(1) + "])";
        if (fn=="from_char")   return "std::string(1," + a(0) + ")";
        if (fn=="from_bool")   return "std::string(" + a(0) + "?\"true\":\"false\")";
        if (fn=="count")       return "__drv_str_count(" + a(0) + ", " + a(1) + ")";
        if (fn=="upper_first") return "__drv_str_upper_first(" + a(0) + ")";
        if (fn=="trim_left")   return "__drv_str_trim_left(" + a(0) + ")";
        if (fn=="trim_right")  return "__drv_str_trim_right(" + a(0) + ")";
        if (fn=="remove")      return "__drv_str_remove(" + a(0) + ", " + a(1) + ")";
        if (fn=="truncate")    return a(0) + ".substr(0, " + a(1) + ")";
    }
    // lst.*
    if (ns=="lst") {
        if (fn=="push")     return a(0) + ".push_back(" + a(1) + ")";
        if (fn=="pop")      return "__drv_lst_pop(" + a(0) + ")";
        if (fn=="size"||fn=="length") return "static_cast<int64_t>(" + a(0) + ".size())";
        if (fn=="get")      return a(0) + "[" + a(1) + "]";
        if (fn=="set")      return "(" + a(0) + "[" + a(1) + "] = " + a(2) + ")";
        if (fn=="insert")   return "(" + a(0) + ".insert(" + a(0) + ".begin()+" + a(1) + ", " + a(2) + "))";
        if (fn=="remove")   return "(" + a(0) + ".erase(" + a(0) + ".begin()+" + a(1) + "))";
        if (fn=="clear")    return a(0) + ".clear()";
        if (fn=="first")    return a(0) + ".front()";
        if (fn=="last")     return a(0) + ".back()";
        if (fn=="is_empty") return a(0) + ".empty()";
        // sort/reverse: in-place (mutate the list)
        if (fn=="sort")     return "(std::sort(" + a(0) + ".begin()," + a(0) + ".end())," + a(0) + ")";
        if (fn=="reverse")  return "(std::reverse(" + a(0) + ".begin()," + a(0) + ".end())," + a(0) + ")";
        if (fn=="contains") return "__drv_lst_contains(" + a(0) + ", " + a(1) + ")";
        if (fn=="min")      return "*std::min_element(" + a(0) + ".begin()," + a(0) + ".end())";
        if (fn=="max")      return "*std::max_element(" + a(0) + ".begin()," + a(0) + ".end())";
        if (fn=="unique")   return "__drv_lst_unique(" + a(0) + ")";
        if (fn=="fill")     return "__drv_lst_fill(" + a(0) + ", " + a(1) + ")";
        if (fn=="copy")     return a(0);
        if (fn=="join")     return "__drv_lst_join(" + a(0) + ", " + a(1) + ")";
        if (fn=="index_of") return "__drv_lst_index_of(" + a(0) + ", " + a(1) + ")";
        if (fn=="count")    return "__drv_lst_count(" + a(0) + ", " + a(1) + ")";
        if (fn=="slice")    return "__drv_lst_slice(" + a(0) + ", " + a(1) + ", " + a(2) + ")";
        if (fn=="extend")   return a(0) + ".insert(" + a(0) + ".end()," + a(1) + ".begin()," + a(1) + ".end())";
    }
    // map.*
    if (ns=="map") {
        if (fn=="set")     return a(0) + "[" + a(1) + "] = " + a(2);
        if (fn=="get")     return a(0) + ".at(" + a(1) + ")";
        if (fn=="get_or")  return "__drv_map_get_or(" + a(0) + ", " + a(1) + ", " + a(2) + ")";
        if (fn=="has")     return "(" + a(0) + ".count(" + a(1) + ")>0)";
        if (fn=="del")     return a(0) + ".erase(" + a(1) + ")";
        if (fn=="size")    return "static_cast<int64_t>(" + a(0) + ".size())";
        if (fn=="clear")   return a(0) + ".clear()";
        if (fn=="keys")    return "__drv_map_keys(" + a(0) + ")";
        if (fn=="values")  return "__drv_map_values(" + a(0) + ")";
        if (fn=="merge")   return "__drv_map_merge(" + a(0) + ", " + a(1) + ")";
        if (fn=="copy")    return a(0);
        if (fn=="contains_value") return "__drv_map_contains_value(" + a(0) + ", " + a(1) + ")";
    }
    // math.*
    if (ns=="math") {
        if (fn=="pow")      return "std::pow(" + a(0) + ", " + a(1) + ")";
        if (fn=="log")      return "std::log(" + a(0) + ")";
        if (fn=="log2")     return "std::log2(" + a(0) + ")";
        if (fn=="log10")    return "std::log10(" + a(0) + ")";
        if (fn=="exp")      return "std::exp(" + a(0) + ")";
        if (fn=="asin")     return "std::asin(" + a(0) + ")";
        if (fn=="acos")     return "std::acos(" + a(0) + ")";
        if (fn=="atan")     return "std::atan(" + a(0) + ")";
        if (fn=="atan2")    return "std::atan2(" + a(0) + ", " + a(1) + ")";
        if (fn=="hypot")    return "std::hypot(" + a(0) + ", " + a(1) + ")";
        if (fn=="clamp")    return "std::clamp(" + a(0) + ", " + a(1) + ", " + a(2) + ")";
        if (fn=="lerp")     return "std::lerp(" + a(0) + ", " + a(1) + ", " + a(2) + ")";
        if (fn=="pi")       return "M_PI";
        if (fn=="e")        return "M_E";
        if (fn=="sign")     return "static_cast<int32_t>((" + a(0) + ">0)-(" + a(0) + "<0))";
        if (fn=="invsqrt")  return "(1.0/std::sqrt(" + a(0) + "))";
        if (fn=="gcd")      return "std::gcd(" + a(0) + ", " + a(1) + ")";
        if (fn=="lcm")      return "std::lcm(" + a(0) + ", " + a(1) + ")";
        if (fn=="factorial") return "__drv_factorial(" + a(0) + ")";
        if (fn=="degrees")  return "(" + a(0) + " * (180.0/M_PI))";
        if (fn=="radians")  return "(" + a(0) + " * (M_PI/180.0))";
        if (fn=="is_nan")   return "std::isnan(" + a(0) + ")";
        if (fn=="is_inf")   return "std::isinf(" + a(0) + ")";
        if (fn=="floor_div") return "static_cast<int64_t>(std::floor((double)" + a(0) + "/" + a(1) + "))";
        if (fn=="sinh")     return "std::sinh(" + a(0) + ")";
        if (fn=="cosh")     return "std::cosh(" + a(0) + ")";
        if (fn=="tanh")     return "std::tanh(" + a(0) + ")";
        if (fn=="fma")      return "std::fma(" + a(0) + ", " + a(1) + ", " + a(2) + ")";
        if (fn=="abs")      return "std::abs(" + a(0) + ")";
        if (fn=="cbrt")     return "std::cbrt(" + a(0) + ")";
    }
    // io.*
    if (ns=="io") {
        if (fn=="read_file")   return "__drv_io_read_file(" + a(0) + ")";
        if (fn=="write_file")  return "__drv_io_write_file(" + a(0) + ", " + a(1) + ")";
        if (fn=="append_file") return "__drv_io_append_file(" + a(0) + ", " + a(1) + ")";
        if (fn=="mmap_read")   return "__drv_io_mmap_read(" + a(0) + ")";
        if (fn=="exists")      return "__drv_io_exists(" + a(0) + ")";
        if (fn=="delete_file") return "__drv_io_delete_file(" + a(0) + ")";
        if (fn=="file_size")   return "__drv_io_file_size(" + a(0) + ")";
        if (fn=="list_dir")    return "__drv_io_list_dir(" + a(0) + ")";
        if (fn=="make_dir")    return "__drv_io_make_dir(" + a(0) + ")";
    }
    // sys.*
    if (ns=="sys") {
        if (fn=="exec")             return "__drv_sys_exec(" + a(0) + ")";
        if (fn=="affinity")         return "__drv_sys_affinity(" + a(0) + ")";
        if (fn=="get_branch_trace") return "__drv_get_branch_trace()";
        if (fn=="clear_branch_trace") return "__drv_clear_branch_trace()";
        if (fn=="env")              return "__drv_sys_env(" + a(0) + ")";
        if (fn=="exit")             return "exit(" + a(0) + ")";
        if (fn=="sleep")            return "__drv_sys_sleep(" + a(0) + ")";
        if (fn=="time")             return "__drv_sys_time()";
    }
    // simd.*
    if (ns=="simd") {
        if (fn=="fmadd") return "std::fma(" + a(0) + ", " + a(1) + ", " + a(2) + ")";
    }
    // mem.*
    if (ns=="mem") {
        if (fn=="prefetch") return "__builtin_prefetch(&(" + a(0) + "), 0, 3)";
    }
    // perf.*
    if (ns=="perf") {
        if (fn=="now") return "__drv_perf_now()";
    }
    // bits.*
    if (ns=="bits") {
        if (fn=="popcount") return "__builtin_popcount(" + a(0) + ")";
    }
    // diff.*
    if (ns=="diff") {
        if (fn=="forward")   return "__drv_diff_forward(" + a(0) + ", " + a(1) + ")";
        if (fn=="numerical") return "__drv_diff_numerical(" + a(0) + ", " + a(1) + ")";
        if (fn=="hessian")   return "__drv_diff_hessian(" + a(0) + ", " + a(1) + ")";
    }
    // reflect.*
    if (ns=="reflect") {
        if (fn=="type_of") return "std::string(typeid(" + a(0) + ").name())";
    }
    // wait.*
    if (ns=="wait") {
        if (fn=="tick")   return "__drv_wait_tick(" + a(0) + ")";
        if (fn=="second") return "__drv_wait_seconds(1)";
        if (fn=="minute") return "__drv_wait_seconds(60)";
        if (fn=="hour")   return "__drv_wait_seconds(3600)";
    }

    return ns + "::" + fn + "(" + as + ")";
}

static std::string argsStr(const ExprList& args, Codegen* cg) {
    std::string s;
    for (size_t i=0; i<args.size(); ++i) {
        if (i) s += ", ";
        s += cg->emitExpr(*args[i]);
    }
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
// Expression emit
// ─────────────────────────────────────────────────────────────────────────────
std::string Codegen::emitExpr(const Expr& e) {
    if (auto p = dynamic_cast<const IntLit*>(&e))    return std::to_string(p->value);
    if (auto p = dynamic_cast<const LongLit*>(&e))   return std::to_string(p->value) + "LL";
    if (auto p = dynamic_cast<const DoubleLit*>(&e)) {
        std::ostringstream ss; ss << std::setprecision(17) << p->value;
        std::string s = ss.str();
        // ensure it has a decimal point so C++ treats it as double, not int
        if (s.find('.') == std::string::npos && s.find('e') == std::string::npos) s += ".0";
        return s;
    }
    if (auto p = dynamic_cast<const FloatLit*>(&e))  { std::ostringstream ss; ss << p->value << "f"; return ss.str(); }
    if (auto p = dynamic_cast<const BoolLit*>(&e))   return p->value ? "true" : "false";
    if (auto p = dynamic_cast<const CharLit*>(&e))   return std::string("'") + p->value + "'";
    if (auto p = dynamic_cast<const StringLit*>(&e)) return "std::string(\"" + p->value + "\")";
    if (dynamic_cast<const NullLit*>(&e))             return "nullptr";
    if (dynamic_cast<const NoneLit*>(&e))             return "std::nullopt";

    if (auto p = dynamic_cast<const IdentExpr*>(&e)) return p->name;
    if (auto p = dynamic_cast<const BinaryExpr*>(&e)) return emitBinary(*p);
    if (auto p = dynamic_cast<const UnaryExpr*>(&e))  return emitUnary(*p);
    if (auto p = dynamic_cast<const CallExpr*>(&e))   return emitCall(*p);
    if (auto p = dynamic_cast<const MemberExpr*>(&e)) return emitMember(*p);
    if (auto p = dynamic_cast<const IndexExpr*>(&e))  return emitIndex(*p);
    if (auto p = dynamic_cast<const LambdaExpr*>(&e)) return emitLambda(*p);
    if (auto p = dynamic_cast<const NewExpr*>(&e))    return emitNew(*p);
    if (auto p = dynamic_cast<const PipeExpr*>(&e))   return emitPipe(*p);
    if (auto p = dynamic_cast<const MoveExpr*>(&e))   return "std::move(" + emitExpr(*p->operand) + ")";
    if (auto p = dynamic_cast<const RangeExpr*>(&e))  return "__drv_range(" + emitExpr(*p->from) + ", " + emitExpr(*p->to) + ")";
    if (auto p = dynamic_cast<const SomeExpr*>(&e))   return emitExpr(*p->value);
    if (auto p = dynamic_cast<const OkExpr*>(&e))     return "Ok(" + emitExpr(*p->value) + ")";
    if (auto p = dynamic_cast<const ErrExpr*>(&e))    return "Err(std::string(" + emitExpr(*p->value) + "))";

    errors_.push_back("unknown expression type in codegen");
    return "/* unknown expr */";
}

std::string Codegen::emitBinary(const BinaryExpr& e) {
    std::string l = emitExpr(*e.left), r = emitExpr(*e.right);
    if (e.op=="and"||e.op=="&&") return "(" + l + " && " + r + ")";
    if (e.op=="or" ||e.op=="||") return "(" + l + " || " + r + ")";
    if (e.op=="not")             return "!" + l;
    if (e.op=="**")              return "std::pow(" + l + ", " + r + ")";
    if (e.op=="//")              return "static_cast<int64_t>(" + l + " / " + r + ")";
    if (e.op=="in")              return "__drv_in(" + l + ", " + r + ")";
    if (e.op=="is")              return "(" + l + " == " + r + ")";
    if (e.op=="as")              return "static_cast<" + r + ">(" + l + ")"; // r is type name
    return "(" + l + " " + e.op + " " + r + ")";
}

std::string Codegen::emitUnary(const UnaryExpr& e) {
    std::string v = emitExpr(*e.operand);
    if (e.op=="not") return "!" + v;
    if (e.prefix)    return e.op + v;
    return v + e.op;
}

std::string Codegen::emitCall(const CallExpr& e) {
    // namespace call: a.b.c(args) — check for member chain that's a namespace
    if (auto mem = dynamic_cast<const MemberExpr*>(e.callee.get())) {
        // check if mem->object is an identifier (namespace)
        if (auto ns_id = dynamic_cast<const IdentExpr*>(mem->object.get())) {
            // It's ns.fn(args) — could be namespace call
            std::string ns = ns_id->name;
            std::string fn = mem->field;
            // Known namespaces
            static const std::vector<std::string> KNOWN_NS =
                {"str","lst","map","math","io","sys","simd","mem","perf","bits","diff","reflect","wait"};
            for (auto& kns : KNOWN_NS) {
                if (ns == kns) return mapBuiltinNamespace(ns, fn, e.args);
            }
        }
    }

    // direct call
    if (auto id = dynamic_cast<const IdentExpr*>(e.callee.get())) {
        return mapBuiltinCall(id->name, e.args);
    }

    // member method call: handle list HOF and other known patterns
    if (auto mem = dynamic_cast<const MemberExpr*>(e.callee.get())) {
        std::string obj = emitExpr(*mem->object);
        std::string fn  = mem->field;
        if (fn == "filter") return "__drv_lst_filter(" + obj + ", " + argsStr(e.args, this) + ")";
        if (fn == "map")    return "__drv_lst_map("    + obj + ", " + argsStr(e.args, this) + ")";
        if (fn == "reduce") {
            auto a = [&](int i) -> std::string { return i < (int)e.args.size() ? emitExpr(*e.args[i]) : ""; };
            return "__drv_lst_reduce(" + obj + ", " + a(0) + ", " + a(1) + ")";
        }
        // smart pointer method call: p.method() → p->method()
        if (auto id = dynamic_cast<const IdentExpr*>(mem->object.get())) {
            if (smart_ptr_vars_.count(id->name))
                return obj + "->" + fn + "(" + argsStr(e.args, this) + ")";
        }
    }
    return emitExpr(*e.callee) + "(" + argsStr(e.args, this) + ")";
}

std::string Codegen::emitMember(const MemberExpr& e) {
    std::string obj = emitExpr(*e.object);
    // Use -> for smart pointer types
    if (auto id = dynamic_cast<const IdentExpr*>(e.object.get())) {
        if (smart_ptr_vars_.count(id->name)) return obj + "->" + e.field;
    }
    return obj + "." + e.field;
}

std::string Codegen::emitIndex(const IndexExpr& e) {
    std::string obj = emitExpr(*e.object);
    std::string idx = emitExpr(*e.index);
    if (e.index2) return obj + "[{" + idx + ", " + emitExpr(*e.index2) + "}]"; // 2D
    return obj + "[" + idx + "]";
}

std::string Codegen::emitLambda(const LambdaExpr& e) {
    std::string cap = "[=]"; // default capture
    if (!e.captures.empty()) {
        cap = "[";
        for (size_t i=0; i<e.captures.size(); ++i) {
            if (i) cap += ", ";
            cap += (e.captures[i].mode=="ref" ? "&" : "") + e.captures[i].var;
        }
        cap += "]";
    }
    std::string params = "(";
    for (size_t i=0; i<e.params.size(); ++i) {
        if (i) params += ", ";
        params += (e.params[i].type.empty() ? "auto" : e.params[i].type) + " " + e.params[i].name;
    }
    params += ")";

    if (e.body_expr) {
        return cap + params + "{ return " + emitExpr(*e.body_expr) + "; }";
    }
    // block body: use sub-codegen to emit stmts as string
    {
        Codegen sub(opts_);
        sub.indent_ = 1;
        for (auto& s : e.body_stmts) sub.emitStmt(*s);
        return cap + params + "{\n" + sub.getOutput() + "}";
    }
    return cap + params + "{ }"; // unreachable
}

std::string Codegen::emitNew(const NewExpr& e) {
    std::string type = mapBuiltinType(e.type_name);
    if (e.array_size) {
        return "std::vector<" + type + ">(" + emitExpr(*e.array_size) + ")";
    }
    return "std::make_shared<" + type + ">(" + argsStr(e.ctor_args, this) + ")";
}

std::string Codegen::emitPipe(const PipeExpr& e) {
    // left |> right  →  right(left)  (if right is lambda or function)
    std::string l = emitExpr(*e.left);
    if (auto lam = dynamic_cast<const LambdaExpr*>(e.right.get())) {
        return emitLambda(*lam) + "(" + l + ")";
    }
    return emitExpr(*e.right) + "(" + l + ")";
}

// ─────────────────────────────────────────────────────────────────────────────
// Annotation handling
// ─────────────────────────────────────────────────────────────────────────────
void Codegen::emitFuncAnnotations(const std::vector<std::string>& annots) {
    for (auto& a : annots) {
        if (a=="@inline")   writeil("[[gnu::always_inline]]");
        if (a=="@noinline") writeil("[[gnu::noinline]]");
        if (a=="@pure")     writeil("[[gnu::pure]]");
        if (a=="@fastcall") writeil("[[gnu::regparm(3)]]");
        if (a=="@noalias")  {} // handled at parameter level
        if (a=="@bench")    {} // injected as RAII timer wrapper
        if (a=="@trace")    {} // injected as branch instrumentation
        if (a=="@threadsafe") writeil("/* @threadsafe */");
        if (a=="@alloc")    {} // documentation only
        if (a=="@io")       {} // documentation only
    }
}

void Codegen::emitVarAnnotations(const std::vector<std::string>& annots, std::string& prefix) {
    for (auto& a : annots) {
        if (a=="@stack") prefix = "/* @stack */ ";
        if (a=="@heap")  prefix = "/* @heap */ ";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Statement emit
// ─────────────────────────────────────────────────────────────────────────────
void Codegen::emitBlock(const StmtList& stmts) {
    writeln("{");
    ++indent_;
    for (auto& s : stmts) emitStmt(*s);
    --indent_;
    writeil("}");
}

void Codegen::emitStmt(const Stmt& s) {
    lineDir(s.line);
    if (auto p = dynamic_cast<const VarDecl*>(&s))      { emitVarDecl(*p); return; }
    if (auto p = dynamic_cast<const FuncDecl*>(&s))     { emitFuncDecl(*p); return; }
    if (auto p = dynamic_cast<const ClassDecl*>(&s))    { emitClassDecl(*p); return; }
    if (auto p = dynamic_cast<const TraitDecl*>(&s))    { emitTraitDecl(*p); return; }
    if (auto p = dynamic_cast<const ImplDecl*>(&s))     { emitImplDecl(*p); return; }
    if (auto p = dynamic_cast<const DimDecl*>(&s))      { emitDimDecl(*p); return; }
    if (auto p = dynamic_cast<const ModuleDecl*>(&s))   { emitModuleDecl(*p); return; }
    if (auto p = dynamic_cast<const UseDecl*>(&s))      { emitUseDecl(*p); return; }
    if (auto p = dynamic_cast<const ExternDecl*>(&s))   { emitExternDecl(*p); return; }
    if (auto p = dynamic_cast<const IfStmt*>(&s))       { emitIfStmt(*p); return; }
    if (auto p = dynamic_cast<const ForRangeStmt*>(&s)) { emitForRange(*p); return; }
    if (auto p = dynamic_cast<const ForEachStmt*>(&s))  { emitForEach(*p); return; }
    if (auto p = dynamic_cast<const ForCStyleStmt*>(&s)){ emitForCStyle(*p); return; }
    if (auto p = dynamic_cast<const WhileStmt*>(&s))    { emitWhileStmt(*p); return; }
    if (auto p = dynamic_cast<const SwitchStmt*>(&s))   { emitSwitchStmt(*p); return; }
    if (auto p = dynamic_cast<const MatchStmt*>(&s))    { emitMatchStmt(*p); return; }
    if (auto p = dynamic_cast<const TryCatchStmt*>(&s)) { emitTryCatch(*p); return; }
    if (auto p = dynamic_cast<const RegionStmt*>(&s))   { emitRegion(*p); return; }
    if (auto p = dynamic_cast<const SpawnStmt*>(&s))    { emitSpawn(*p); return; }
    if (auto p = dynamic_cast<const StaticIfStmt*>(&s)) { emitStaticIf(*p); return; }
    if (auto p = dynamic_cast<const ReturnStmt*>(&s))   {
        writei("return");
        if (p->value) write(" " + emitExpr(*p->value));
        writeln(";");
        return;
    }
    if (dynamic_cast<const BreakStmt*>(&s))    { writeil("break;");    return; }
    if (dynamic_cast<const ContinueStmt*>(&s)) { writeil("continue;"); return; }
    if (dynamic_cast<const PassStmt*>(&s))     { writeil("/* pass */;"); return; }
    if (auto p = dynamic_cast<const ThrowStmt*>(&s)) {
        writeil("throw std::runtime_error(" + emitExpr(*p->value) + ");");
        return;
    }
    if (auto p = dynamic_cast<const ExprStmt*>(&s)) {
        writeil(emitExpr(*p->expr) + ";");
        return;
    }
}

void Codegen::emitVarDecl(const VarDecl& s) {
    std::string prefix;
    emitVarAnnotations(s.annotations, prefix);

    // Track smart pointer variables
    bool is_own = (s.type.name == "Own");
    bool is_ref = (s.type.name == "Ref");
    if (is_own) smart_ptr_vars_[s.name] = "own";
    if (is_ref) smart_ptr_vars_[s.name] = "ref";

    std::string type = mapType(s.type);
    std::string init_str;
    if (s.init) {
        // For Own<T>/Ref<T> with NewExpr, use the right make_ function
        if (auto ne = dynamic_cast<const NewExpr*>(s.init.get())) {
            std::string t = mapBuiltinType(ne->type_name);
            std::string ctor_args = argsStr(ne->ctor_args, this);
            if (is_own)      init_str = " = std::make_unique<" + t + ">(" + ctor_args + ")";
            else if (is_ref) init_str = " = std::make_shared<" + t + ">(" + ctor_args + ")";
            else             init_str = " = " + emitExpr(*s.init);
        } else {
            init_str = " = " + emitExpr(*s.init);
        }
    }
    writeil(prefix + type + " " + s.name + init_str + ";");
}

void Codegen::emitFuncDecl(const FuncDecl& s) {
    // annotations
    emitFuncAnnotations(s.annotations);

    // build signature
    bool is_bench = std::find(s.annotations.begin(), s.annotations.end(), "@bench") != s.annotations.end();
    bool is_trace = std::find(s.annotations.begin(), s.annotations.end(), "@trace") != s.annotations.end();
    bool is_compile_eval = std::find(s.modifiers.begin(), s.modifiers.end(), "compile_eval") != s.modifiers.end();

    std::string ret = mapType(s.ret_type);
    if (s.is_async) ret = "std::future<" + ret + ">";

    std::string sig = (is_compile_eval ? "constexpr " : "") + ret + " " + s.name;
    if (!s.type_params.empty()) {
        sig = "template<";
        for (size_t i=0; i<s.type_params.size(); ++i) {
            if (i) sig += ", ";
            sig += "typename " + s.type_params[i];
        }
        sig += ">\n" + ret + " " + s.name;
    }
    sig += "(";
    for (size_t i=0; i<s.params.size(); ++i) {
        if (i) sig += ", ";
        sig += mapType(s.params[i].type) + " " + s.params[i].name;
        if (s.params[i].default_val) sig += " = " + emitExpr(*s.params[i].default_val);
    }
    sig += ")";

    if (s.is_async) {
        // async fn → return std::async(launch::async, [=]() -> RetType { body })
        std::string inner_ret = mapType(s.ret_type);
        writeil(sig + " {");
        ++indent_;
        // capture params by value
        std::string cap = "[";
        for (size_t i = 0; i < s.params.size(); ++i) {
            if (i) cap += ", ";
            cap += s.params[i].name;
        }
        cap += "]";
        writeil("return std::async(std::launch::async, " + cap + "() -> " + inner_ret + " {");
        ++indent_;
        for (auto& stmt : s.body) emitStmt(*stmt);
        --indent_;
        writeil("});");
        --indent_;
        writeil("}");
        writeln();
        return;
    }

    writeil(sig + " {");
    ++indent_;

    if (is_bench) {
        writeil("double __drv_bench_start__ = __drv_perf_now();");
        writeil("struct __DrvBenchGuard__ {");
        writeil("    double start; const char* name;");
        writeil("    ~__DrvBenchGuard__() { fprintf(stderr, \"[bench] %s: %.3f ms\\n\", name, __drv_perf_now() - start); }");
        writeil("} __drv_bench_guard__ = { __drv_bench_start__, \"" + s.name + "\" };");
    }

    bool prev_tracing = tracing_;
    std::string prev_func = tracing_func_;
    if (is_trace) { tracing_ = true; tracing_func_ = s.name; }
    for (auto& stmt : s.body) emitStmt(*stmt);
    tracing_ = prev_tracing; tracing_func_ = prev_func;
    --indent_;
    writeil("}");
    writeln();
}

void Codegen::emitClassDecl(const ClassDecl& s) {
    bool is_packed = std::find(s.annotations.begin(),s.annotations.end(),"@packed")!=s.annotations.end();

    // @align(N) → alignas(N)
    std::string align_attr;
    for (auto& a : s.annotations) {
        if (a.size() > 7 && a.substr(0,7) == "@align(") {
            std::string n = a.substr(7, a.size()-8); // strip @align( and )
            align_attr = "alignas(" + n + ") ";
            break;
        }
    }

    if (is_packed) writeil("#pragma pack(push, 1)");

    // Build base list: explicit extends + trait bases from impl blocks
    std::string decl = align_attr + "struct " + s.name;
    std::vector<std::string> bases;
    if (!s.base.empty()) bases.push_back("public " + s.base);
    auto it = impls_by_class_.find(s.name);
    if (it != impls_by_class_.end()) {
        for (auto* impl : it->second)
            bases.push_back("public " + impl->trait_name);
    }
    if (!bases.empty()) {
        decl += " : ";
        for (size_t i = 0; i < bases.size(); ++i) {
            if (i) decl += ", ";
            decl += bases[i];
        }
    }
    writeil(decl + " {");
    ++indent_;

    for (auto& m : s.methods) {
        emitStmt(*m);
    }

    // Inject methods from impl blocks
    if (it != impls_by_class_.end()) {
        for (auto* impl : it->second) {
            for (auto& m : impl->methods) emitStmt(*m);
        }
    }

    --indent_;
    writeil("};");
    if (is_packed) writeil("#pragma pack(pop)");
    writeln();
}

void Codegen::emitTraitDecl(const TraitDecl& s) {
    writeil("struct " + s.name + " {");
    ++indent_;
    for (auto& m : s.body) {
        if (auto fn = dynamic_cast<const FuncDecl*>(m.get())) {
            if (fn->body.empty()) {
                // abstract method — emit as pure virtual
                std::string sig = "virtual " + mapType(fn->ret_type) + " " + fn->name + "(";
                for (size_t i = 0; i < fn->params.size(); ++i) {
                    if (i) sig += ", ";
                    sig += mapType(fn->params[i].type) + " " + fn->params[i].name;
                }
                sig += ") = 0;";
                writeil(sig);
            } else {
                // default implementation
                emitStmt(*m);
            }
        } else {
            emitStmt(*m);
        }
    }
    writeil("virtual ~" + s.name + "() = default;");
    --indent_;
    writeil("};");
    writeln();
}

void Codegen::emitImplDecl(const ImplDecl& s) {
    // Methods already injected into class body via emitClassDecl — nothing to emit here
    (void)s;
}

void Codegen::emitDimDecl(const DimDecl& s) {
    writeil("// dim " + s.name + " = \"" + s.symbol + "\"");
    writeil("struct __dim_" + s.name + " {");
    writeil("    double value;");
    writeil("    explicit __dim_" + s.name + "(double v) : value(v) {}");
    writeil("    __dim_" + s.name + " operator+(__dim_" + s.name + " o) const { return __dim_" + s.name + "(value+o.value); }");
    writeil("    __dim_" + s.name + " operator-(__dim_" + s.name + " o) const { return __dim_" + s.name + "(value-o.value); }");
    writeil("    __dim_" + s.name + " operator*(double f) const { return __dim_" + s.name + "(value*f); }");
    writeil("    double operator/(__dim_" + s.name + " o) const { return value/o.value; }");
    writeil("    __dim_" + s.name + " operator/(double f) const { return __dim_" + s.name + "(value/f); }");
    writeil("};");
    writeil("using " + s.name + " = __dim_" + s.name + ";");
    writeln();
}

void Codegen::emitModuleDecl(const ModuleDecl& s) {
    writeil("// module " + s.name);
    writeln();
}

void Codegen::emitUseDecl(const UseDecl& s) {
    if (!s.symbol.empty())
        writeil("// use " + s.module + "::" + s.symbol);
    else
        writeil("// use " + s.module);
    writeln();
}

void Codegen::emitExternDecl(const ExternDecl& s) {
    if (s.abi == "C" || s.abi == "\"C\"") {
        writeil("extern \"C\" {");
    } else if (s.abi == "FFI" || s.abi == "\"FFI\"") {
        writeil("// extern FFI");
        writeil("extern \"C\" {  // FFI bridge (auto-mapped)");
    } else {
        writeil("extern \"C\" {  // " + s.abi);
    }
    if (s.unsafe_legacy) writeil("  // @unsafe_legacy: signal handlers auto-injected");
    ++indent_;
    for (auto& m : s.declarations) {
        // In extern block, FuncDecl emits as forward declaration only (no body)
        if (auto fn = dynamic_cast<const FuncDecl*>(m.get())) {
            std::string sig = mapType(fn->ret_type) + " " + fn->name + "(";
            for (size_t i = 0; i < fn->params.size(); ++i) {
                if (i) sig += ", ";
                sig += mapType(fn->params[i].type) + " " + fn->params[i].name;
            }
            sig += ");";
            writeil(sig);
        } else {
            emitStmt(*m);
        }
    }
    --indent_;
    writeil("}");
    writeln();
}

static std::string stripOuterParens(const std::string& s) {
    if (s.size() >= 2 && s.front() == '(' && s.back() == ')') return s.substr(1, s.size()-2);
    return s;
}

void Codegen::emitIfStmt(const IfStmt& s) {
    std::string cond_str = stripOuterParens(emitExpr(*s.cond));
    writei("if (" + cond_str + ") ");
    if (tracing_) {
        // inject trace call at start of then-block
        writeln("{");
        ++indent_;
        writeil("__drv_trace_push(\"" + tracing_func_ + "\", \"if:" + cond_str + "\", " + std::to_string(s.line) + ");");
        for (auto& st : s.then_body) emitStmt(*st);
        --indent_;
        writeil("}");
    } else {
        emitBlock(s.then_body);
    }
    if (!s.else_body.empty()) {
        writei("else ");
        if (tracing_) {
            writeln("{");
            ++indent_;
            writeil("__drv_trace_push(\"" + tracing_func_ + "\", \"else\", " + std::to_string(s.line) + ");");
            for (auto& st : s.else_body) emitStmt(*st);
            --indent_;
            writeil("}");
        } else {
            emitBlock(s.else_body);
        }
    }
    writeln();
}

void Codegen::emitForRange(const ForRangeStmt& s) {
    std::string from = emitExpr(*s.from);
    std::string to   = s.to ? emitExpr(*s.to) : from;

    // reduction preamble
    std::vector<std::pair<std::string,std::string>> reds; // op, var
    for (auto& r : s.reductions) {
        auto colon = r.find(':');
        if (colon != std::string::npos)
            reds.push_back({r.substr(0,colon), r.substr(colon+1)});
    }

    if (s.is_parallel) {
        // emit as OpenMP parallel for (simplified)
        for (auto& [op,var] : reds)
            writeil("#pragma omp parallel for reduction(" + op + ":" + var + ")");
        writeil("#pragma omp parallel for");
    } else if (s.is_simd) {
        writeil("#pragma GCC ivdep");
        writeil("#pragma omp simd");
    }

    writei("for (auto " + s.var + " = " + from + "; " + s.var + " < " + to + "; ++" + s.var + ") ");
    emitBlock(s.body);
    writeln();
}

void Codegen::emitForEach(const ForEachStmt& s) {
    std::string coll = emitExpr(*s.collection);

    if (!s.reductions.empty()) {
        for (auto& r : s.reductions) {
            auto colon = r.find(':');
            if (colon != std::string::npos) {
                std::string op = r.substr(0,colon);
                std::string var = r.substr(colon+1);
                writeil("#pragma omp parallel for reduction(" + op + ":" + var + ")");
            }
        }
    }

    if (s.is_parallel) writeil("#pragma omp parallel for");
    if (s.is_simd)     writeil("#pragma GCC ivdep");

    writei("for (auto& " + s.var + " : " + coll + ") ");
    emitBlock(s.body);
    writeln();
}

void Codegen::emitForCStyle(const ForCStyleStmt& s) {
    // simplified: emit init; cond; step
    writei("for (");
    if (s.init) write(/* strip trailing ; */ "");
    write("; ");
    if (s.cond) write(emitExpr(*s.cond));
    write("; ");
    if (s.step) write(emitExpr(*s.step));
    write(") ");
    emitBlock(s.body);
    writeln();
}

void Codegen::emitWhileStmt(const WhileStmt& s) {
    writei("while (" + stripOuterParens(emitExpr(*s.cond)) + ") ");
    emitBlock(s.body);
    writeln();
}

void Codegen::emitSwitchStmt(const SwitchStmt& s) {
    writeil("switch (" + emitExpr(*s.expr) + ") {");
    ++indent_;
    for (auto& arm : s.arms) {
        if (arm.is_default) writeil("default:");
        else writeil("case (" + emitExpr(*arm.value) + "):");
        ++indent_;
        for (auto& st : arm.body) emitStmt(*st);
        writeil("break;");
        --indent_;
    }
    --indent_;
    writeil("}");
    writeln();
}

void Codegen::emitMatchStmt(const MatchStmt& s) {
    std::string expr = emitExpr(*s.expr);
    writeil("// match " + expr);
    writeil("{");
    ++indent_;
    writeil("auto __match_val__ = " + expr + ";");
    bool first = true;
    for (auto& arm : s.arms) {
        std::string cond;
        if (arm.pattern == "_") {
            cond = "true";
        } else if (arm.pattern == "Some" || arm.pattern == "Ok") {
            cond = "__match_val__.has_value()";
        } else if (arm.pattern == "None") {
            cond = "!__match_val__.has_value()";
        } else if (arm.pattern == "Err") {
            cond = "!__match_val__.has_value()";
        } else {
            cond = "(__match_val__ == " + arm.pattern + ")";
        }

        writei((first ? "if" : "else if") + std::string(" (") + cond + ") ");
        first = false;

        writeln("{");
        ++indent_;
        if (!arm.bind_var.empty()) {
            if (arm.pattern == "Some" || arm.pattern == "Ok")
                writeil("auto " + arm.bind_var + " = __match_val__.value();");
            else if (arm.pattern == "Err")
                writeil("auto " + arm.bind_var + " = __match_val__.error();");
        }
        for (auto& st : arm.body) emitStmt(*st);
        --indent_;
        writeil("}");
    }
    --indent_;
    writeil("}");
    writeln();
}

void Codegen::emitTryCatch(const TryCatchStmt& s) {
    writei("try ");
    emitBlock(s.try_body);
    if (!s.catch_body.empty()) {
        writei("catch (const std::exception& __ex__) ");
        writeln("{");
        ++indent_;
        writeil("std::string " + s.catch_var + " = __ex__.what();");
        for (auto& st : s.catch_body) emitStmt(*st);
        --indent_;
        writeil("}");
    }
    writeln();
}

void Codegen::emitRegion(const RegionStmt& s) {
    writeil("// @region " + s.name);
    writeil("{");
    ++indent_;
    for (auto& st : s.body) emitStmt(*st);
    --indent_;
    writeil("} // end @region " + s.name);
    writeln();
}

void Codegen::emitSpawn(const SpawnStmt& s) {
    writeil("std::thread([&]() {");
    ++indent_;
    for (auto& st : s.body) emitStmt(*st);
    --indent_;
    writeil("}).detach();");
    writeln();
}

void Codegen::emitStaticIf(const StaticIfStmt& s) {
    // If condition is a simple identifier, use #ifdef/#endif (works at any scope)
    // Otherwise use if constexpr (function scope only)
    bool use_ifdef = dynamic_cast<const IdentExpr*>(s.cond.get()) != nullptr;
    if (use_ifdef) {
        std::string flag = dynamic_cast<const IdentExpr*>(s.cond.get())->name;
        writeil("#ifdef " + flag);
        for (auto& st : s.then_body) emitStmt(*st);
        if (!s.else_body.empty()) {
            writeil("#else");
            for (auto& st : s.else_body) emitStmt(*st);
        }
        writeil("#endif");
    } else {
        writeil("if constexpr (" + emitExpr(*s.cond) + ") {");
        ++indent_;
        for (auto& st : s.then_body) emitStmt(*st);
        --indent_;
        if (!s.else_body.empty()) {
            writeil("} else {");
            ++indent_;
            for (auto& st : s.else_body) emitStmt(*st);
            --indent_;
        }
        writeil("}");
    }
    writeln();
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers: classify top-level statements
// ─────────────────────────────────────────────────────────────────────────────
// Returns true if expr (or any sub-expr) is a lambda or pipe
static bool exprHasLambda(const Expr* e) {
    if (!e) return false;
    if (dynamic_cast<const LambdaExpr*>(e)) return true;
    if (auto p = dynamic_cast<const PipeExpr*>(e))
        return exprHasLambda(p->left.get()) || exprHasLambda(p->right.get());
    if (auto p = dynamic_cast<const BinaryExpr*>(e))
        return exprHasLambda(p->left.get()) || exprHasLambda(p->right.get());
    return false;
}

static bool varDeclNeedsEntry(const VarDecl& vd) {
    if (vd.type.name == "var") return true;  // auto — needs entry scope
    if (exprHasLambda(vd.init.get())) return true;
    return false;
}

static bool isGlobalDecl(const Stmt& s) {
    return dynamic_cast<const FuncDecl*>(&s)   ||
           dynamic_cast<const ClassDecl*>(&s)  ||
           dynamic_cast<const TraitDecl*>(&s)  ||
           dynamic_cast<const ImplDecl*>(&s)   ||
           dynamic_cast<const DimDecl*>(&s)    ||
           dynamic_cast<const ModuleDecl*>(&s) ||
           dynamic_cast<const UseDecl*>(&s)    ||
           dynamic_cast<const ExternDecl*>(&s);
    // Note: StaticIfStmt goes into __drv_entry__() — not file scope
}

// ─────────────────────────────────────────────────────────────────────────────
// Top-level emit
// ─────────────────────────────────────────────────────────────────────────────
std::string Codegen::emit(const Program& prog) {
    // ── standard headers ─────────────────────────────────────────────────────
    writeln("// Generated by drv compiler — DO NOT EDIT");
    writeln("// Source: " + opts_.source_file);
    writeln("#include <cstdint>");
    writeln("#include <string>");
    writeln("#include <vector>");
    writeln("#include <unordered_map>");
    writeln("#include <memory>");
    writeln("#include <optional>");
    writeln("#include <variant>");
    writeln("#include <functional>");
    writeln("#include <thread>");
    writeln("#include <future>");
    writeln("#include <algorithm>");
    writeln("#include <numeric>");
    writeln("#include <cmath>");
    writeln("#include <iostream>");
    writeln("#include <fstream>");
    writeln("#include <sstream>");
    writeln("#include <stdexcept>");
    writeln("#include <chrono>");
    writeln("#include <cstdio>");
    writeln("#include <cstdlib>");
    writeln("#include <cassert>");
    writeln("#ifndef M_PI");
    writeln("#define M_PI 3.14159265358979323846");
    writeln("#endif");
    writeln("#ifndef M_E");
    writeln("#define M_E  2.71828182845904523536");
    writeln("#endif");
    writeln();

    // ── drv Result<T> / Option<T> ─────────────────────────────────────────────
    writeln("// ── drv Result<T> (C++20 compatible) ─────────────────────────────");
    writeln("struct __DrvErr { std::string msg; };  // type-erased error wrapper");
    writeln("template<typename T>");
    writeln("struct DrvResult {");
    writeln("    std::variant<T, std::string> data_;");
    writeln("    bool ok_;");
    writeln("    DrvResult(std::variant<T,std::string> d, bool o) : data_(d), ok_(o) {}");
    writeln("    DrvResult(__DrvErr e) : data_(e.msg), ok_(false) {}");
    writeln("    static DrvResult Ok(T v)              { return {std::variant<T,std::string>(v), true}; }");
    writeln("    static DrvResult Err(std::string msg) { return {std::variant<T,std::string>(msg), false}; }");
    writeln("    bool has_value() const   { return ok_; }");
    writeln("    T value()        const   { return std::get<T>(data_); }");
    writeln("    std::string error() const { return std::get<std::string>(data_); }");
    writeln("};");
    writeln("template<typename T> using Option = std::optional<T>;");
    writeln("template<typename T> using Result = DrvResult<T>;");
    writeln("template<typename T> auto Some(T v)         { return std::optional<T>(v); }");
    writeln("static auto None = std::nullopt;");
    writeln("template<typename T> DrvResult<T> Ok(T v)   { return DrvResult<T>::Ok(v); }");
    writeln("inline __DrvErr Err(std::string e)           { return {e}; }  // implicit-converts to any DrvResult<T>");
    writeln();

    // ── drv runtime helpers ───────────────────────────────────────────────────
    writeln("// ── drv runtime ──────────────────────────────────────────────────");
    writeln("template<typename T> static std::string __drv_to_str(const T& v) {");
    writeln("    if constexpr (std::is_same_v<T,bool>) return v ? \"true\" : \"false\";");
    writeln("    else { std::ostringstream os; os << v; return os.str(); }");
    writeln("}");
    writeln("static void __drv_print() { std::cout << '\\n'; }");
    writeln("template<typename A, typename... Rest>");
    writeln("static void __drv_print(A&& a, Rest&&... rest) {");
    writeln("    std::cout << __drv_to_str(a);");
    writeln("    if constexpr (sizeof...(rest) > 0) std::cout << ' ';");
    writeln("    __drv_print(std::forward<Rest>(rest)...);");
    writeln("}");
    writeln("template<typename T>");
    writeln("static int64_t __drv_length(const std::vector<T>& v) { return (int64_t)v.size(); }");
    writeln("static int64_t __drv_length(const std::string& s)    { return (int64_t)s.size(); }");
    writeln("static double __drv_perf_now() {");
    writeln("    using namespace std::chrono;");
    writeln("    return duration<double,std::milli>(steady_clock::now().time_since_epoch()).count();");
    writeln("}");
    writeln("static void __drv_assert_that(bool c, const std::string& m) {");
    writeln("    if (!c) throw std::runtime_error(\"Assertion failed: \" + m);");
    writeln("}");
    writeln("template<typename T>");
    writeln("static bool __drv_in(const T& v, const std::vector<T>& c) {");
    writeln("    return std::find(c.begin(),c.end(),v)!=c.end();");
    writeln("}");
    writeln("static std::vector<std::string> __drv_split(const std::string& s, const std::string& d) {");
    writeln("    std::vector<std::string> r; size_t p=0,f;");
    writeln("    while((f=s.find(d,p))!=std::string::npos){r.push_back(s.substr(p,f-p));p=f+d.size();}");
    writeln("    r.push_back(s.substr(p)); return r;");
    writeln("}");
    writeln("static std::string __drv_toUpper(std::string s){for(auto&c:s)c=toupper(c);return s;}");
    writeln("static std::string __drv_toLower(std::string s){for(auto&c:s)c=tolower(c);return s;}");
    writeln("static std::string __drv_trim(std::string s){");
    writeln("    auto b=s.find_first_not_of(\" \\t\\n\\r\");");
    writeln("    auto e=s.find_last_not_of(\" \\t\\n\\r\");");
    writeln("    return b==std::string::npos?\"\":s.substr(b,e-b+1);");
    writeln("}");
    writeln("static std::string __drv_replace(std::string s,const std::string&o,const std::string&n){");
    writeln("    size_t p=0; while((p=s.find(o,p))!=std::string::npos){s.replace(p,o.size(),n);p+=n.size();}return s;");
    writeln("}");
    writeln("// Branch trace");
    writeln("thread_local std::vector<std::string> __drv_branch_trace__;");
    writeln("static std::vector<std::string> __drv_get_branch_trace(){return __drv_branch_trace__;}");
    writeln("static void __drv_clear_branch_trace(){__drv_branch_trace__.clear();}");
    writeln("static void __drv_trace_push(const char*fn,const char*c,int l){");
    writeln("    __drv_branch_trace__.push_back(std::string(fn)+\" | \"+c+\" | line \"+std::to_string(l));");
    writeln("}");
    writeln("// I/O");
    writeln("static std::string __drv_io_read_file(const std::string&p){");
    writeln("    std::ifstream f(p);return{std::istreambuf_iterator<char>(f),{}};");
    writeln("}");
    writeln("static bool __drv_io_write_file(const std::string&p,const std::string&c){");
    writeln("    std::ofstream f(p);f<<c;return f.good();");
    writeln("}");
    writeln("static bool __drv_io_append_file(const std::string&p,const std::string&c){");
    writeln("    std::ofstream f(p,std::ios::app);f<<c;return f.good();");
    writeln("}");
    writeln("static bool __drv_io_exists(const std::string&p){std::ifstream f(p);return f.good();}");
    writeln("static int64_t __drv_io_file_size(const std::string&p){");
    writeln("    std::ifstream f(p,std::ios::ate|std::ios::binary);");
    writeln("    if(!f)return -1;");
    writeln("    return static_cast<int64_t>(f.tellg());");
    writeln("}");
    writeln("static bool __drv_io_delete_file(const std::string&p){return std::remove(p.c_str())==0;}");
    writeln("static bool __drv_io_make_dir(const std::string&p){return std::system((\"mkdir -p \"+p).c_str())==0;}");
    writeln("// Sys");
    writeln("static int __drv_sys_exec(const std::string&c){return std::system(c.c_str());}");
    writeln("#ifdef _MSC_VER");
    writeln("static std::string __drv_sys_env(const std::string&n){");
    writeln("    char*v=nullptr;size_t s=0;_dupenv_s(&v,&s,n.c_str());");
    writeln("    std::string r=v?v:\"\";free(v);return r;}");
    writeln("#else");
    writeln("static std::string __drv_sys_env(const std::string&n){");
    writeln("    const char*v=std::getenv(n.c_str());return v?v:\"\";}");
    writeln("#endif");
    writeln("static void __drv_sys_sleep(int64_t ms){");
    writeln("    std::this_thread::sleep_for(std::chrono::milliseconds(ms));}");
    writeln("static int64_t __drv_sys_time(){");
    writeln("    return std::chrono::duration_cast<std::chrono::seconds>(");
    writeln("        std::chrono::system_clock::now().time_since_epoch()).count();}");
    writeln("// Collections");
    writeln("template<typename T>");
    writeln("static T __drv_lst_pop(std::vector<T>&v){auto r=v.back();v.pop_back();return r;}");
    writeln("template<typename T>");
    writeln("static std::vector<T> __drv_lst_sort(std::vector<T> v){std::sort(v.begin(),v.end());return v;}");
    writeln("template<typename T>");
    writeln("static std::vector<T> __drv_lst_reverse(std::vector<T> v){std::reverse(v.begin(),v.end());return v;}");
    writeln("template<typename T>");
    writeln("static bool __drv_lst_contains(const std::vector<T>&v,const T&x){");
    writeln("    return std::find(v.begin(),v.end(),x)!=v.end();}");
    writeln("template<typename T>");
    writeln("static int64_t __drv_lst_index_of(const std::vector<T>&v,const T&x){");
    writeln("    auto it=std::find(v.begin(),v.end(),x);return it==v.end()?-1:it-v.begin();}");
    writeln("template<typename T>");
    writeln("static int64_t __drv_lst_count(const std::vector<T>&v,const T&x){");
    writeln("    return std::count(v.begin(),v.end(),x);}");
    writeln("template<typename T>");
    writeln("static std::string __drv_lst_join(const std::vector<T>&v,const std::string&sep){");
    writeln("    std::string r;for(size_t i=0;i<v.size();i++){if(i)r+=sep;r+=v[i];}return r;}");
    writeln("template<typename T>");
    writeln("static std::vector<T> __drv_lst_slice(const std::vector<T>&v,int64_t a,int64_t b){");
    writeln("    return{v.begin()+a,v.begin()+b};}");
    writeln("template<typename T>");
    writeln("static std::vector<T> __drv_lst_fill(int64_t n,T val){return std::vector<T>(n,val);}");
    writeln("template<typename T>");
    writeln("static std::vector<T> __drv_lst_unique(std::vector<T> v){");
    writeln("    std::vector<T>r;for(auto&x:v)if(!__drv_lst_contains(r,x))r.push_back(x);return r;}");
    writeln("template<typename K,typename V>");
    writeln("static V __drv_map_get_or(const std::unordered_map<K,V>&m,const K&k,V def){");
    writeln("    auto it=m.find(k);return it!=m.end()?it->second:def;}");
    writeln("template<typename K,typename V>");
    writeln("static std::vector<K> __drv_map_keys(const std::unordered_map<K,V>&m){");
    writeln("    std::vector<K>r;for(auto&p:m)r.push_back(p.first);return r;}");
    writeln("template<typename K,typename V>");
    writeln("static std::vector<V> __drv_map_values(const std::unordered_map<K,V>&m){");
    writeln("    std::vector<V>r;for(auto&p:m)r.push_back(p.second);return r;}");
    writeln("template<typename K,typename V>");
    writeln("static bool __drv_map_contains_value(const std::unordered_map<K,V>&m,const V&v){");
    writeln("    for(auto&p:m)if(p.second==v)return true;return false;}");
    writeln("template<typename K,typename V>");
    writeln("static std::unordered_map<K,V> __drv_map_merge(std::unordered_map<K,V> a,const std::unordered_map<K,V>&b){");
    writeln("    for(auto&p:b)a[p.first]=p.second;return a;}");
    writeln("// Math extras");
    writeln("static int64_t __drv_factorial(int64_t n){int64_t r=1;for(int64_t i=2;i<=n;i++)r*=i;return r;}");
    writeln("static std::vector<std::string> __drv_range(int64_t a,int64_t b){");
    writeln("    std::vector<std::string>r;for(auto i=a;i<b;i++)r.push_back(std::to_string(i));return r;}");
    writeln("// Str extras");
    writeln("static std::string __drv_str_reverse(std::string s){std::reverse(s.begin(),s.end());return s;}");
    writeln("static std::string __drv_str_repeat(const std::string&s,int64_t n){");
    writeln("    std::string r;for(int64_t i=0;i<n;i++)r+=s;return r;}");
    writeln("static std::string __drv_str_pad_left(const std::string&s,int64_t n){");
    writeln("    if((int64_t)s.size()>=n)return s;return std::string(n-s.size(),' ')+s;}");
    writeln("static std::string __drv_str_pad_right(const std::string&s,int64_t n){");
    writeln("    if((int64_t)s.size()>=n)return s;return s+std::string(n-s.size(),' ');}");
    writeln("static int64_t __drv_str_count(const std::string&s,const std::string&sub){");
    writeln("    int64_t c=0;size_t p=0;while((p=s.find(sub,p))!=std::string::npos){c++;p+=sub.size();}return c;}");
    writeln("static std::string __drv_str_upper_first(std::string s){if(!s.empty())s[0]=toupper(s[0]);return s;}");
    writeln("static std::string __drv_str_trim_left(std::string s){");
    writeln("    auto b=s.find_first_not_of(\" \\t\");return b==std::string::npos?\"\":s.substr(b);}");
    writeln("static std::string __drv_str_trim_right(std::string s){");
    writeln("    auto e=s.find_last_not_of(\" \\t\");return e==std::string::npos?\"\":s.substr(0,e+1);}");
    writeln("static std::string __drv_str_remove(std::string s,const std::string&sub){");
    writeln("    size_t p;while((p=s.find(sub))!=std::string::npos)s.erase(p,sub.size());return s;}");
    writeln("static std::string __drv_str_join(const std::vector<std::string>&v,const std::string&sep){");
    writeln("    return __drv_lst_join(v,sep);}");
    writeln("// Collection HOF: map / filter / reduce");
    writeln("template<typename T, typename F>");
    writeln("static auto __drv_lst_map(const std::vector<T>& v, F f) {");
    writeln("    std::vector<decltype(f(v[0]))> r; r.reserve(v.size());");
    writeln("    for (auto& x : v) r.push_back(f(x)); return r; }");
    writeln("template<typename T, typename F>");
    writeln("static std::vector<T> __drv_lst_filter(const std::vector<T>& v, F f) {");
    writeln("    std::vector<T> r;");
    writeln("    for (auto& x : v) if (f(x)) r.push_back(x); return r; }");
    writeln("template<typename T, typename A, typename F>");
    writeln("static A __drv_lst_reduce(const std::vector<T>& v, A acc, F f) {");
    writeln("    for (auto& x : v) acc = f(acc, x); return acc; }");
    writeln("// Diff stubs");
    writeln("static double __drv_diff_forward(std::function<double(double)> f,double x){");
    writeln("    const double h=1e-7;return(f(x+h)-f(x))/h;}");
    writeln("static double __drv_diff_numerical(std::function<double(double)> f,double x){");
    writeln("    const double h=1e-5;return(f(x+h)-f(x-h))/(2*h);}");
    writeln("static double __drv_diff_hessian(std::function<double(double)> f,double x){");
    writeln("    const double h=1e-4;return(f(x+h)-2*f(x)+f(x-h))/(h*h);}");
    writeln("// Tensor stats");
    writeln("static double __drv_sum(const std::vector<double>&v){double s=0;for(auto x:v)s+=x;return s;}");
    writeln("static double __drv_mean(const std::vector<double>&v){return v.empty()?0:__drv_sum(v)/v.size();}");
    writeln("static double __drv_norm(const std::vector<double>&v){double s=0;for(auto x:v)s+=x*x;return std::sqrt(s);}");
    writeln("static double __drv_dot(const std::vector<double>&a,const std::vector<double>&b){");
    writeln("    double s=0;for(size_t i=0;i<std::min(a.size(),b.size());i++)s+=a[i]*b[i];return s;}");
    writeln("// ─────────────────────────────────────────────────────────────────");
    writeln();

    // ── Pre-pass: collect impl blocks for class injection ─────────────────────
    impls_by_class_.clear();
    for (auto& s : prog.stmts) {
        if (auto impl = dynamic_cast<const ImplDecl*>(s.get())) {
            impls_by_class_[impl->class_name].push_back(impl);
        }
    }

    // ── Pass 1: global declarations only (func/class/trait/etc.) ────────────
    bool has_main = false;
    bool has_entry_stmts = false;
    for (auto& s : prog.stmts) {
        if (isGlobalDecl(*s)) {
            emitStmt(*s);
        } else {
            has_entry_stmts = true; // VarDecls + ExprStmts → entry
        }
        if (auto fn = dynamic_cast<const FuncDecl*>(s.get()))
            if (fn->name == "main") has_main = true;
    }
    writeln();

    // ── Pass 2: everything else → __drv_entry__() ────────────────────────────
    if (has_entry_stmts) {
        writeln("static void __drv_entry__() {");
        ++indent_;
        for (auto& s : prog.stmts) {
            if (isGlobalDecl(*s)) continue;
            emitStmt(*s);
        }
        --indent_;
        writeln("}");
        writeln();
    }

    // ── main() ────────────────────────────────────────────────────────────────
    if (!has_main) {
        writeln("int main(int argc, char* argv[]) {");
        if (has_entry_stmts) writeln("    __drv_entry__();");
        writeln("    return 0;");
        writeln("}");
    }

    return out_.str();
}

} // namespace drv
