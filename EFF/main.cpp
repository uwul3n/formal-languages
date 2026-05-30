#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <set>
#include <map>

using Symbol = std::string;
using Sentence = std::vector<Symbol>;

struct Grammar {
    std::set<Symbol> terminals;
    std::set<Symbol> nonterminals;
    Symbol start;
    std::vector<std::pair<Symbol, Sentence>> rules;
};

static bool isNonterm(const Grammar& g, const Symbol& s) {
    return g.nonterminals.count(s) > 0;
}

static std::set<Sentence> concatK(const std::set<Sentence>& A,
                                  const std::set<Sentence>& B, int k) {
    std::set<Sentence> res;
    for (const auto& a : A) for (const auto& b : B) {
        Sentence c = a;
        for (const auto& s : b) { if ((int)c.size() >= k) break; c.push_back(s); }
        res.insert(c);
    }
    return res;
}

static std::set<Sentence> firstK(const Grammar& g, const Sentence& alpha, int k,
                                 const std::map<Symbol, std::set<Sentence>>& firstNT) {
    std::set<Sentence> res = {Sentence{}};
    for (const auto& X : alpha) {
        std::set<Sentence> fx;
        if (isNonterm(g, X)) { auto it = firstNT.find(X); if (it != firstNT.end()) fx = it->second; }
        else fx = {Sentence{X}};
        if (fx.empty()) { res.clear(); break; }
        res = concatK(res, fx, k);
        bool allFull = !res.empty();
        for (const auto& s : res) if ((int)s.size() < k) { allFull = false; break; }
        if (allFull) break;
    }
    return res;
}

static void computeFirstKNT(const Grammar& g, int k,
                            std::map<Symbol, std::set<Sentence>>& firstNT) {
    for (const auto& A : g.nonterminals) firstNT[A] = {};
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& [A, gamma] : g.rules) {
            auto f = firstK(g, gamma, k, firstNT);
            for (const auto& s : f) if (firstNT[A].insert(s).second) changed = true;
        }
    }
}

static std::set<Sentence> effK(const Grammar& g, const Sentence& alpha, int k,
                               const std::map<Symbol, std::set<Sentence>>& firstNT) {
    if (alpha.empty()) return {};
    if (!isNonterm(g, alpha[0])) return firstK(g, alpha, k, firstNT);
    std::set<Sentence> result, visited;
    std::vector<Sentence> queue; queue.push_back(alpha); visited.insert(alpha);
    const size_t MAX_LEN = std::max<size_t>(64, alpha.size() + 32);
    while (!queue.empty()) {
        Sentence form = queue.back(); queue.pop_back();
        if (form.empty()) continue;
        if (!isNonterm(g, form[0])) {
            auto f = firstK(g, form, k, firstNT);
            result.insert(f.begin(), f.end());
            continue;
        }
        const Symbol& A = form[0];
        Sentence tail(form.begin()+1, form.end());
        for (const auto& [L, gamma] : g.rules) {
            if (L != A) continue;
            if (gamma.empty()) continue; // не раскрываем ведущий нетерминал через ε
            Sentence next = gamma;
            next.insert(next.end(), tail.begin(), tail.end());
            if (next.size() > MAX_LEN) continue;
            if (visited.insert(next).second) queue.push_back(next);
        }
    }
    return result;
}

static std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> out; std::istringstream iss(line); std::string t;
    while (iss >> t) out.push_back(t);
    return out;
}

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "Usage: " << argv[0] << " <input_file>\n"; return 1; }
    std::ifstream in(argv[1]);
    if (!in) { std::cerr << "Cannot open " << argv[1] << "\n"; return 1; }

    std::string line;
    auto nextLine = [&](std::string& out) -> bool {
        while (std::getline(in, line)) {
            size_t p = line.find_first_not_of(" \t\r\n");
            if (p == std::string::npos) continue;
            if (line[p] == '#') continue;
            out = line; return true;
        }
        return false;
    };

    Grammar g;
    std::string l;
    if (!nextLine(l)) { std::cerr << "Missing terminals\n"; return 1; }
    for (auto& t : tokenize(l)) g.terminals.insert(t);
    if (!nextLine(l)) { std::cerr << "Missing nonterminals\n"; return 1; }
    for (auto& t : tokenize(l)) g.nonterminals.insert(t);
    if (!nextLine(l)) { std::cerr << "Missing start\n"; return 1; }
    g.start = tokenize(l)[0];
    if (!nextLine(l)) { std::cerr << "Missing rule count\n"; return 1; }
    int nRules = std::stoi(tokenize(l)[0]);
    for (int i = 0; i < nRules; i++) {
        if (!nextLine(l)) { std::cerr << "Missing rule " << i << "\n"; return 1; }
        auto toks = tokenize(l);
        if (toks.size() < 2 || toks[1] != "->") { std::cerr << "Bad rule: " << l << "\n"; return 1; }
        Symbol A = toks[0]; Sentence rhs;
        for (size_t j = 2; j < toks.size(); j++) {
            if (toks[j] == "eps" || toks[j] == "ε") continue;
            rhs.push_back(toks[j]);
        }
        g.rules.push_back({A, rhs});
    }
    if (!nextLine(l)) { std::cerr << "Missing k\n"; return 1; }
    int k = std::stoi(tokenize(l)[0]);

    std::map<Symbol, std::set<Sentence>> firstNT;
    computeFirstKNT(g, k, firstNT);

    std::cout << "k = " << k << "\n";
    while (nextLine(l)) {
        Sentence alpha;
        for (auto& t : tokenize(l)) { if (t == "eps" || t == "ε") continue; alpha.push_back(t); }
        auto res = effK(g, alpha, k, firstNT);
        std::cout << "EFF_" << k << "(";
        for (size_t i = 0; i < alpha.size(); i++) { if (i) std::cout << " "; std::cout << alpha[i]; }
        std::cout << ") = {";
        bool first = true;
        for (const auto& s : res) {
            if (!first) std::cout << ", "; first = false;
            if (s.empty()) std::cout << "ε";
            else for (size_t i = 0; i < s.size(); i++) { if (i) std::cout << " "; std::cout << s[i]; }
        }
        std::cout << "}\n";
    }
    return 0;
}
