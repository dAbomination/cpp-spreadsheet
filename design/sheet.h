#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>

class Sheet : public SheetInterface {
public:
    ~Sheet();
	
    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::unordered_map<int, std::unordered_map<int, Cell*>> row_col_cell_;
    // -1, -1 ������ ��� ���� ����(��� ��� � ��������� ���������� ���������� � 0, 0)
    Size printable_size_{-1, -1};

    // ������� ����� � �������� ������� ���� � ������� � ��������������� ����  
    std::map<int, std::set<int>> rows_cols_numbers_;    

    // ��������� ���������� �� ������
    bool CheckCell(Position pos) const;
    // ��������� �� ���������� �������
    void IsValidPos(Position pos) const;  
    // �������� ��������� �������
    template <typename Func>
    void PrintSheet(std::ostream& output, Func& f) const;
};

template <typename Func>
void Sheet::PrintSheet(std::ostream& output, Func& f) const {

    for (int y = 0; y <= printable_size_.rows; ++y) {
        for (int x = 0; x <= printable_size_.cols; ++x) {
            if (CheckCell({ y,x })) {
                output << f(y, x);
            }
            if (!(x == printable_size_.cols)) {
                output << '\t';
            }
        }
        output << '\n';
    }
}