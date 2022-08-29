#pragma once

#include "common.h"
#include "formula.h"

namespace CellImpl {

    class Impl {
    public:		
        Impl(std::string_view str);

        std::string GetText() const;
        virtual CellInterface::Value GetValue() const = 0;
		// Инвалидирует значение формульных значений
		virtual void InvalidateValue() = 0;
		void Evaluate();
		
		bool IsValueValid() const;
    protected:
        std::string text_;		
    };

    // Пустая ячейка
    class EmptyImpl : public Impl {
    public:
        EmptyImpl();
        CellInterface::Value GetValue() const override;
    };

    // Текстовая ячейка
    class TextImpl : public Impl {
    public:
        TextImpl(std::string_view str);

        CellInterface::Value GetValue() const override;
    private:
        std::string value_;
		// Возможно ли представить текст ячейки в качестве числа
		bool is_number_;
		
		bool IsNumber() const;
    };

    // Ячейка с формулой
    class FormulaImpl : public Impl {
    public:		
        FormulaImpl(std::string_view str, SheetInterface& sheet);
		// Рассчитывает значение формулы
		void Evaluate();
		
        CellInterface::Value GetValue() const override;
		// Возвращает индексы всех ячеек, которые входят в формулу
		std::vector<Position> GetReferencedCells() const;
		// Определяет валидность ячейки
		bool IsValueValid() const;
    private:
		// Если значение есть значит ячейка валидна, при инвалидациия значение очищается
        std::optional<std::variant<double, FormulaError>> value_;			
    };

} // namespace CellImpl

class Cell : public CellInterface {
public:
    Cell();
    ~Cell();
	// f - функция для получения значения данных в таблице, где находится данная ячейка	
    void Set(std::string text, SheetInterface& sheet);
    void Clear();
	
    CellInterface::Value GetValue() const override;
    std::string GetText() const override;
	
	std::vector<Position> GetReferencedCells() const override;
	
	void Evaluate();
	// Инвалидирует значение всех ячеек ссылающихся на данную
	void InvalidateRederringCells();
	// Добавляет ячейку которая ссылается на текущую
	void AddReferringCell(Position pos);
	// Проверяет на валидность impl_
	bool IsDataValid() const;
private:
    std::unique_ptr<CellImpl::Impl> impl_;	
	// Вектор с позициями ячеек, задействованных в данной формуле
	std::vector<Cell*> cell_references_;	
	// Вектор ячеек в которых используется данная ячейка
	std::vector<Cell*> referring_cells_;
};