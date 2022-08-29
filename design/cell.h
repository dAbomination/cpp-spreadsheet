#pragma once

#include "common.h"
#include "formula.h"

namespace CellImpl {

    class Impl {
    public:		
        Impl(std::string_view str);

        std::string GetText() const;
        virtual CellInterface::Value GetValue() const = 0;
		// ������������ �������� ���������� ��������
		virtual void InvalidateValue() = 0;
		void Evaluate();
		
		bool IsValueValid() const;
    protected:
        std::string text_;		
    };

    // ������ ������
    class EmptyImpl : public Impl {
    public:
        EmptyImpl();
        CellInterface::Value GetValue() const override;
    };

    // ��������� ������
    class TextImpl : public Impl {
    public:
        TextImpl(std::string_view str);

        CellInterface::Value GetValue() const override;
    private:
        std::string value_;
		// �������� �� ����������� ����� ������ � �������� �����
		bool is_number_;
		
		bool IsNumber() const;
    };

    // ������ � ��������
    class FormulaImpl : public Impl {
    public:		
        FormulaImpl(std::string_view str, SheetInterface& sheet);
		// ������������ �������� �������
		void Evaluate();
		
        CellInterface::Value GetValue() const override;
		// ���������� ������� ���� �����, ������� ������ � �������
		std::vector<Position> GetReferencedCells() const;
		// ���������� ���������� ������
		bool IsValueValid() const;
    private:
		// ���� �������� ���� ������ ������ �������, ��� ������������ �������� ���������
        std::optional<std::variant<double, FormulaError>> value_;			
    };

} // namespace CellImpl

class Cell : public CellInterface {
public:
    Cell();
    ~Cell();
	// f - ������� ��� ��������� �������� ������ � �������, ��� ��������� ������ ������	
    void Set(std::string text, SheetInterface& sheet);
    void Clear();
	
    CellInterface::Value GetValue() const override;
    std::string GetText() const override;
	
	std::vector<Position> GetReferencedCells() const override;
	
	void Evaluate();
	// ������������ �������� ���� ����� ����������� �� ������
	void InvalidateRederringCells();
	// ��������� ������ ������� ��������� �� �������
	void AddReferringCell(Position pos);
	// ��������� �� ���������� impl_
	bool IsDataValid() const;
private:
    std::unique_ptr<CellImpl::Impl> impl_;	
	// ������ � ��������� �����, ��������������� � ������ �������
	std::vector<Cell*> cell_references_;	
	// ������ ����� � ������� ������������ ������ ������
	std::vector<Cell*> referring_cells_;
};