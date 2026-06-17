#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <initializer_list>
#include <print>
#include <cctype>

// ============================================================================
// Abstract Base Class: Sentence
// ============================================================================

class Sentence {
public:
    virtual ~Sentence() = default;

    virtual bool evaluate(const std::unordered_map<std::string, bool>& model) const = 0;
    virtual std::string formula() const = 0;
    virtual std::unordered_set<std::string> symbols() const = 0;
    virtual std::string repr() const = 0;

    static bool balanced(std::string_view s) {
        int count = 0;
        for (char c : s) {
            if (c == '(') {
                count++;
            } else if (c == ')') {
                if (count <= 0) return false;
                count--;
            }
        }
        return count == 0;
    }

    static std::string parenthesize(const std::string& s) {
        if (s.empty()) return s;

        bool is_alpha = std::all_of(s.begin(), s.end(), [](unsigned char c) {
            return std::isalpha(c);
        });

        if (is_alpha) return s;

        if (s.front() == '(' && s.back() == ')') {
            std::string_view inner(s.data() + 1, s.size() - 2);
            if (balanced(inner)) return s;
        }

        return "(" + s + ")";
    }

    static void validate(const std::shared_ptr<Sentence>& sentence) {
        if (!sentence) {
            throw std::invalid_argument("must be a logical sentence");
        }
    }
};

// ============================================================================
// Logical Sentence Subclasses
// ============================================================================

class Symbol : public Sentence {
public:
    std::string name;

    Symbol(std::string name) : name(std::move(name)) {}

    bool evaluate(const std::unordered_map<std::string, bool>& model) const override {
        if (auto it = model.find(name); it != model.end()) {
            return it->second;
        }
        throw std::runtime_error("variable " + name + " not in model");
    }

    std::string formula() const override { return name; }

    std::unordered_set<std::string> symbols() const override { return {name}; }

    std::string repr() const override { return name; }
};

class Not : public Sentence {
public:
    std::shared_ptr<Sentence> operand;

    Not(std::shared_ptr<Sentence> operand) : operand(std::move(operand)) {
        Sentence::validate(this->operand);
    }

    bool evaluate(const std::unordered_map<std::string, bool>& model) const override {
        return !operand->evaluate(model);
    }

    std::string formula() const override {
        return "¬" + Sentence::parenthesize(operand->formula());
    }

    std::unordered_set<std::string> symbols() const override {
        return operand->symbols();
    }

    std::string repr() const override {
        return "Not(" + operand->repr() + ")";
    }
};

class And : public Sentence {
public:
    std::vector<std::shared_ptr<Sentence>> conjuncts;

    And(std::initializer_list<std::shared_ptr<Sentence>> args) : conjuncts(args) {
        for (const auto& conjunct : conjuncts) {
            Sentence::validate(conjunct);
        }
    }

    void add(std::shared_ptr<Sentence> conjunct) {
        Sentence::validate(conjunct);
        conjuncts.push_back(std::move(conjunct));
    }

    bool evaluate(const std::unordered_map<std::string, bool>& model) const override {
        return std::all_of(conjuncts.begin(), conjuncts.end(), [&](const auto& c) {
            return c->evaluate(model);
        });
    }

    std::string formula() const override {
        if (conjuncts.empty()) return "";
        if (conjuncts.size() == 1) return conjuncts[0]->formula();

        std::string result = Sentence::parenthesize(conjuncts[0]->formula());
        for (size_t i = 1; i < conjuncts.size(); ++i) {
            result += " ∧ " + Sentence::parenthesize(conjuncts[i]->formula());
        }
        return result;
    }

    std::unordered_set<std::string> symbols() const override {
        std::unordered_set<std::string> total_symbols;
        for (const auto& conjunct : conjuncts) {
            auto syms = conjunct->symbols();
            total_symbols.insert(syms.begin(), syms.end());
        }
        return total_symbols;
    }

    std::string repr() const override {
        std::string result = "And(";
        for (size_t i = 0; i < conjuncts.size(); ++i) {
            result += conjuncts[i]->repr();
            if (i + 1 < conjuncts.size()) result += ", ";
        }
        result += ")";
        return result;
    }
};

class Or : public Sentence {
public:
    std::vector<std::shared_ptr<Sentence>> disjuncts;

    Or(std::initializer_list<std::shared_ptr<Sentence>> args) : disjuncts(args) {
        for (const auto& disjunct : disjuncts) {
            Sentence::validate(disjunct);
        }
    }

    bool evaluate(const std::unordered_map<std::string, bool>& model) const override {
        return std::any_of(disjuncts.begin(), disjuncts.end(), [&](const auto& d) {
            return d->evaluate(model);
        });
    }

    std::string formula() const override {
        if (disjuncts.empty()) return "";
        if (disjuncts.size() == 1) return disjuncts[0]->formula();

        std::string result = Sentence::parenthesize(disjuncts[0]->formula());
        for (size_t i = 1; i < disjuncts.size(); ++i) {
            result += " ∨  " + Sentence::parenthesize(disjuncts[i]->formula());
        }
        return result;
    }

    std::unordered_set<std::string> symbols() const override {
        std::unordered_set<std::string> total_symbols;
        for (const auto& disjunct : disjuncts) {
            auto syms = disjunct->symbols();
            total_symbols.insert(syms.begin(), syms.end());
        }
        return total_symbols;
    }

    std::string repr() const override {
        std::string result = "Or(";
        for (size_t i = 0; i < disjuncts.size(); ++i) {
            result += disjuncts[i]->repr();
            if (i + 1 < disjuncts.size()) result += ", ";
        }
        result += ")";
        return result;
    }
};

class Implication : public Sentence {
public:
    std::shared_ptr<Sentence> antecedent;
    std::shared_ptr<Sentence> consequent;

    Implication(std::shared_ptr<Sentence> ant, std::shared_ptr<Sentence> cons)
        : antecedent(std::move(ant)), consequent(std::move(cons)) {
        Sentence::validate(antecedent);
        Sentence::validate(consequent);
    }

    bool evaluate(const std::unordered_map<std::string, bool>& model) const override {
        return !antecedent->evaluate(model) || consequent->evaluate(model);
    }

    std::string formula() const override {
        std::string ant = Sentence::parenthesize(antecedent->formula());
        std::string cons = Sentence::parenthesize(consequent->formula());
        return ant + " => " + cons;
    }

    std::unordered_set<std::string> symbols() const override {
        auto total_symbols = antecedent->symbols();
        auto cons_syms = consequent->symbols();
        total_symbols.insert(cons_syms.begin(), cons_syms.end());
        return total_symbols;
    }

    std::string repr() const override {
        return "Implication(" + antecedent->repr() + ", " + consequent->repr() + ")";
    }
};

class Biconditional : public Sentence {
public:
    std::shared_ptr<Sentence> left;
    std::shared_ptr<Sentence> right;

    Biconditional(std::shared_ptr<Sentence> l, std::shared_ptr<Sentence> r)
        : left(std::move(l)), right(std::move(r)) {
        Sentence::validate(left);
        Sentence::validate(right);
    }

    bool evaluate(const std::unordered_map<std::string, bool>& model) const override {
        bool l_val = left->evaluate(model);
        bool r_val = right->evaluate(model);
        return (l_val && r_val) || (!l_val && !r_val);
    }

    std::string formula() const override {
        // Matches Python script's choice to capture the string representation/repr inside formula
        std::string l_str = Sentence::parenthesize(left->repr());
        std::string r_str = Sentence::parenthesize(right->repr());
        return l_str + " <=> " + r_str;
    }

    std::unordered_set<std::string> symbols() const override {
        auto total_symbols = left->symbols();
        auto r_syms = right->symbols();
        total_symbols.insert(r_syms.begin(), r_syms.end());
        return total_symbols;
    }

    std::string repr() const override {
        return "Biconditional(" + left->repr() + ", " + right->repr() + ")";
    }
};

// ============================================================================
// Model Checking Core Algorithm
// ============================================================================

bool check_all(
    const std::shared_ptr<Sentence>& knowledge,
    const std::shared_ptr<Sentence>& query,
    std::unordered_set<std::string> symbols,
    std::unordered_map<std::string, bool> model) 
{
    // If model has an assignment for each symbol
    if (symbols.empty()) {
        // If knowledge base is true in model, then query must also be true
        if (knowledge->evaluate(model)) {
            return query->evaluate(model);
        }
        return true;
    } else {
        // Choose one of the remaining unused symbols
        auto it = symbols.begin();
        std::string p = *it;
        symbols.erase(it);

        // Create a model where the symbol is true
        auto model_true = model;
        model_true[p] = true;

        // Create a model where the symbol is false
        auto model_false = model;
        model_false[p] = false;

        // Ensure entailment holds in both models
        return check_all(knowledge, query, symbols, model_true) &&
               check_all(knowledge, query, symbols, model_false);
    }
}

bool model_check(const std::shared_ptr<Sentence>& knowledge, const std::shared_ptr<Sentence>& query) {
    // Get all symbols in both knowledge and query
    auto symbols = knowledge->symbols();
    auto query_symbols = query->symbols();
    symbols.insert(query_symbols.begin(), query_symbols.end());

    // Check that knowledge entails query
    return check_all(knowledge, query, symbols, {});
}

// ============================================================================
// Main Execution / Sanity Verification
// ============================================================================

int main() {
    // Example Setup: 
    // Knowledge Base: (A => B) ∧ A
    // Query: B
    auto symA = std::make_shared<Symbol>("A");
    auto symB = std::make_shared<Symbol>("B");

    auto KB = std::make_shared<And>(std::initializer_list<std::shared_ptr<Sentence>>{
        std::make_shared<Implication>(symA, symB),
        symA
    });

    std::println("Knowledge Base Formula: {}", KB->formula());
    std::println("Query Formula: {}", symB->formula());

    if (model_check(KB, symB)) {
        std::println("Success: The Knowledge Base entails the query!");
    } else {
        std::println("Failure: The Knowledge Base does NOT entail the query.");
    }

    return 0;
}
