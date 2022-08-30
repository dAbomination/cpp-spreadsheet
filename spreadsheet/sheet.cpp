#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {    
}

bool Sheet::CheckCell(Position pos) const {
    auto row = row_col_cell_.find(pos.row);
    if (row != row_col_cell_.end() && row->second.find(pos.col) != row->second.end()) {
        return true;
    }
    else {
        return false;
    }
}

void Sheet::IsValidPos(Position pos) const {
     if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position!");
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    IsValidPos(pos);
    
    if (CheckCell(pos)) {
        static_cast<Cell*>(GetCell(pos))->Set(text);        
    }
    else {
        auto new_cell_ptr = std::make_unique<Cell>(text, *this, pos);
        row_col_cell_[pos.row][pos.col] = std::move(new_cell_ptr);

        // Если какой-либо размер новой ячейки больше печатной области, изменяем печатную область
        if (pos.col > printable_size_.cols) {
            printable_size_.cols = pos.col;
        }
        if (pos.row > printable_size_.rows) {
            printable_size_.rows = pos.row;
        }
        rows_cols_numbers_[pos.row].insert(pos.col);
    }    
}

const CellInterface* Sheet::GetCell(Position pos) const {
    IsValidPos(pos);
    
    if (CheckCell(pos)) {
        return row_col_cell_.at(pos.row).at(pos.col).get();
    }
    else {
        return nullptr;
    }
}

CellInterface* Sheet::GetCell(Position pos) {
    IsValidPos(pos);

    if (CheckCell(pos)) {
        return row_col_cell_[pos.row][pos.col].get();
    }
    else {
        return nullptr;
    }
}

void Sheet::ClearCell(Position pos) {
    IsValidPos(pos);

    if (CheckCell(pos)) {
        row_col_cell_[pos.row].erase(pos.col);
        rows_cols_numbers_[pos.row].erase(pos.col);

        // Если в текущей строке не осталось элменетов, удаляем её
        if (row_col_cell_[pos.row].empty()) {
            row_col_cell_.erase(pos.row);
            rows_cols_numbers_.erase(pos.row);                      
        }
    }  
    // Таблица опустела
    if (rows_cols_numbers_.empty()) {
        printable_size_ = { -1, -1 };        
    }
    else {
        // Если позиция удаляемой ячейки равна координатам печатной области
        // необходимо найти следующие за ней координаты
        if (pos.row == printable_size_.rows) {
            printable_size_.rows = rows_cols_numbers_.rbegin()->first;
        }
        if (pos.col == printable_size_.cols) {
            int max_col = 0;

            // Проходим по всем строкам находя максимальный индекс столбца
            std::for_each(rows_cols_numbers_.begin(),
                rows_cols_numbers_.end(),
                [&max_col](const auto& row) {
                    if (*row.second.rbegin() > max_col) {
                        max_col = *row.second.rbegin();
                    }
                }
            );
            printable_size_.cols = max_col;
        }
    } 
}

Size Sheet::GetPrintableSize() const {
    return { printable_size_.rows + 1, printable_size_.cols + 1 };
}

struct value_output {
    std::string operator()(const std::string& value) const {
        return value;
    }

    std::string operator()(const double value) const {
        std::stringstream temp;
        temp << value;
        return temp.str();
    }

    std::string operator()(const FormulaError& x) const {
        return std::string(x.ToString());
    }
};

void Sheet::PrintValues(std::ostream& output) const {
    auto get_value = [this](int x, int y) {
        const auto value = GetCell(Position{ x, y })->GetValue();        
        return std::visit(value_output(), value);
    };
    PrintSheet(output, get_value);
}

void Sheet::PrintTexts(std::ostream& output) const {
    auto get_text = [this](int x, int y) {
        return GetCell(Position{ x, y })->GetText();
    };
    PrintSheet(output, get_text);
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}