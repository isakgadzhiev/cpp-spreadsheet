#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <set>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#DIV/0!";
}

namespace {
    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression)
                : ast_(ParseFormulaAST(expression)) {
        }

        Value Evaluate(const SheetInterface& sheet) const override {
            try {
                auto func = [&sheet, this] (Position pos) -> FormulaInterface::Value {
                    if (pos < Position{sheet.GetPrintableSize().rows, sheet.GetPrintableSize().cols}) {
                        auto cell_ptr = sheet.GetCell(pos);
                        if (!cell_ptr) {
                            return 0.0;
                        }
                        CellInterface::Value value = cell_ptr->GetValue();
                        if (std::holds_alternative<std::string>(value)) {
                            return StringToDouble(std::get<std::string>(value));
                        } else if (std::holds_alternative<double>(value)) {
                            return std::get<double>(value);
                        } else {
                            return std::get<FormulaError>(value);
                        }
                    } else if (pos.IsValid()) {
                        return 0.0;
                    } else {
                        throw FormulaError(FormulaError::Category::Ref);
                    }
                };
                return ast_.Execute(func);
            } catch (const FormulaError& what) {
                return what;
            }
        }

        std::string GetExpression() const override {
            std::ostringstream result;
            ast_.PrintFormula(result);
            return result.str();
        }

        std::vector<Position> GetReferencedCells() const override {
            std::vector<Position> result = {};
            if (!ast_.GetCells().empty()) {
                for (const auto& cell : ast_.GetCells()) {
                    if (result.empty()) {
                        result.emplace_back(cell);
                    } else if (result.back() != cell) {
                        result.emplace_back(cell);
                    }
                }
            }
            return result;
        }

    private:
        FormulaAST ast_;

        Value StringToDouble(std::string text) const {
            if (text.empty()) {
                return 0.0;
            } else {
                try {
                    double number = std::stod(text);
                    return number;
                } catch (...) {
                    return FormulaError(FormulaError::Category::Value);
                }
            }
        }
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (...) {
        throw FormulaException("Incorrect formula");
    }
}