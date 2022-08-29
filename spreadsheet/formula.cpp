#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#DIV/0!";
}

namespace {
    class Formula : public FormulaInterface {
    public:
        // Реализуйте следующие методы:
        explicit Formula(std::string expression)
            : ast_(ParseFormulaAST(expression)) {            
        }

        Value Evaluate(const SheetInterface& sheet) const override {
            // Лямба для получения значения по позиции, если ячейки не существует возвращает 0
            auto cell_func = [&sheet](Position pos) {
                if (sheet.GetCell(pos) == nullptr) {
                    CellInterface::Value temp = 0.0;
                    return temp;
                }
                else {
                    return sheet.GetCell(pos)->GetValue();
                }                
            };
            
            try {
                return ast_.Execute(cell_func);
            }
            catch (FormulaError& er) {
                return er;
            }
        }

        std::string GetExpression() const override {
            std::ostringstream out;            
            ast_.PrintFormula(out);
            return out.str();
        }

        bool operator<(const Position& rhs) const {
            return false;
        }

        std::vector<Position> GetReferencedCells() const {
            std::vector<Position> temp_cells = { ast_.GetCells().begin(), ast_.GetCells().end() };
            std::sort(temp_cells.begin(), temp_cells.end());
            temp_cells.erase(std::unique(temp_cells.begin(), temp_cells.end()), temp_cells.end());

            return temp_cells;
        }

    private:
        FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    std::unique_ptr<FormulaInterface> result;
    try {
        result = std::make_unique<Formula>(std::move(expression));
        return result;
    }
    catch (...) {
        throw FormulaException("");
    }
}