#pragma once

#include "common.h"
#include "formula.h"

#include <optional>

namespace CellImpl {

    class Impl {
    public:
        Impl(std::string_view str);

        const std::string& GetText() const;
        virtual CellInterface::Value GetValue() const = 0;        
        virtual std::vector<Position> GetReferencedCells() const;        
    protected:
        std::string text_;
    };

    // Пустая ячейка
    class EmptyImpl : public Impl {
    public:
        EmptyImpl();
        CellInterface::Value GetValue() const override;
    };

    // Текстовая ¤чейка
    class TextImpl : public Impl {
    public:
        TextImpl(std::string_view str);
        CellInterface::Value GetValue() const override;
    private:
        std::variant<std::string, double> value_;
        // Возможно ли представить текст ячейки в качестве числа
        bool is_number_ = false;
    };

    // Ячейка с формулой
    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string_view str, SheetInterface& sheet, Position self);
        void Evaluate(SheetInterface& sheet);
        void Invalidate();

        bool IsValid() const;

        CellInterface::Value GetValue() const override;
        std::vector<Position> GetReferencedCells() const;
    private:
        std::unique_ptr<FormulaInterface> formula_;
        Position self_;
        // Если значение есть значит ячейка валидна, при инвалидации¤ значение очищаетс¤
        std::optional<std::variant<double, FormulaError>> value_; 

        void CheckCircular(const std::vector<Position>& positions, SheetInterface& sheet) const;
    };

} // namespace CellImpl

class Cell : public CellInterface {
public:
    Cell(std::string_view text, SheetInterface& sheet, Position self);
    ~Cell();

    void Set(std::string text);
    void Clear();

    CellInterface::Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;  

    std::vector<Position>& GetReferringCells();
    // Пересчитывает значение ячейки
    void ReEvaluate();
        
private:
    SheetInterface& sheet_;
    Position self_; // Позиция ячейки в таблице
    std::unique_ptr<CellImpl::Impl> impl_;

    // Вектор ячеек в которых используется данная ячейка
    std::vector<Position> referring_cells_;

    // Пересчёт ячеек, в которых используется данная ячейка
    void ReEvaluateReferringCells();
    // Создаёт экземпляр Impl  в зависимости от text
    void CreateImpl(std::string_view text);
    // Инвалидирует все ячейки которые используют данную
    void InvalidateReferringCells();
    void Invalidate();

    bool IsFormulaImpl();
    bool IsValid();
};
