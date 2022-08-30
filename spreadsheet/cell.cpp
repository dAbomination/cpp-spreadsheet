#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

namespace CellImpl {

	Impl::Impl(std::string_view str)
		: text_(str) {
	}

	const std::string& Impl::GetText() const {
		return text_;
	}

	std::vector<Position> Impl::GetReferencedCells() const {
		return {};
	}
	// --------------------------------------------------------------------

	EmptyImpl::EmptyImpl()
		: Impl("") {
	}

	CellInterface::Value EmptyImpl::GetValue() const {
		return "";
	}

	// --------------------------------------------------------------------

	TextImpl::TextImpl(std::string_view str)
		: Impl(str) {
		if (str.size() > 0 && str[0] == ESCAPE_SIGN) {
			str.remove_prefix(1);
		}
		// Определяем что строка может быть числом
		if (!str.empty() && str.find_first_not_of("0123456789.") == std::string_view::npos) {
			value_ = std::stod(std::string(str));
			is_number_ = true;
		}
		else {
			value_ = std::string(str);
		}
	}

	CellInterface::Value TextImpl::GetValue() const {
		if (is_number_) {
			return std::get<double>(value_);
		}
		else {
			return std::get<std::string>(value_);
		}
	}

	// --------------------------------------------------------------------
	template <class T>
	inline void hash_combine(std::size_t& s, const T& v) {
		std::hash<T> h;
		s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
	}

	struct PositionHash {	
		std::size_t operator()(Position const& pos) const
		{
			std::size_t res = 0;
			hash_combine(res, pos.col);
			hash_combine(res, pos.row);
			return res;
		}
	};
	
	void FormulaImpl::CheckCircular(const std::vector<Position>& positions, SheetInterface& sheet) const {
		std::unordered_set<Position, PositionHash> checked_positions;
				
		for (const auto& pos : positions) {
			// Такую позицию ещё не проверяли
			if (checked_positions.count(pos) == 0) {
				auto temp_cell = sheet.GetCell(pos);
				// Ячейка за границами таблицы
				if (temp_cell == nullptr) {
					continue;
				}

				if (pos == self_) {
					throw CircularDependencyException("");
				}
				checked_positions.insert(pos);				
				CheckCircular(temp_cell->GetReferencedCells(), sheet);
			}			
		}
	}

	FormulaImpl::FormulaImpl(std::string_view str, SheetInterface& sheet, Position self)
		: Impl(str), self_(self) {
		try {
			formula_ = ParseFormula(std::string(str.begin() + 1, str.end()));
			// Введена синтаксически неверная формула, значение не меняем			
		}
		catch (.../*FormulaException& e*/) {			
			throw FormulaException("");
		}

		bool valid_references = true;
		// Проверям что все ячейки на которые ссылается формула могут
		// быть представлены как числа, существуют а их позиции верны
		const auto referenced_cell = GetReferencedCells();
		for (const Position& pos : referenced_cell) {
			auto temp_cell = static_cast<Cell*>(sheet.GetCell(pos));

			if (!temp_cell) {
				sheet.SetCell(pos, "");
				continue;
			}			
			if (!std::holds_alternative<double>(temp_cell->GetValue())) {				
				value_ = FormulaError(FormulaError::Category::Value);
				valid_references = false;
			}		
			temp_cell->GetReferringCells().push_back(self);
		}

		// Проверям что нет цикличной зависимости
		CheckCircular(GetReferencedCells(), sheet);

		if (valid_references) {
			Evaluate(sheet);
		}
	}

	CellInterface::Value FormulaImpl::GetValue() const {
		if (std::holds_alternative<FormulaError>(value_.value())) {
			return std::get<FormulaError>(value_.value());
		}
		return std::get<double>(value_.value());		
	}

	std::vector<Position> FormulaImpl::GetReferencedCells() const {
		if (!formula_) {
			return {};
		}
		return formula_->GetReferencedCells();
	}

	void FormulaImpl::Evaluate(SheetInterface& sheet) {		
		text_ = FORMULA_SIGN + formula_->GetExpression();
		auto result = formula_->Evaluate(sheet);
		// Формула успешно посчиталась
		if (std::holds_alternative<double>(result)) {
			value_ = std::get<double>(result);
		}
		// Ошибка при вычислении
		else {
			value_ = std::get<FormulaError>(result);
		}
	}

	void FormulaImpl::Invalidate() {
		value_ = std::nullopt;
	}

	bool FormulaImpl::IsValid() const {
		return value_.has_value();
	}
} // namespace CellImpl

// ----------------------------- Cell ------------------------------------------

Cell::Cell(std::string_view text, SheetInterface& sheet, Position self)
	: sheet_(sheet), self_(self) {
	CreateImpl(text);
}

void Cell::Set(std::string text) {
	CreateImpl(text);
	// Пересчитываем эту ячейку и другие, которые используют данные этой ячейки
	// Чтобы пересчитывать только 1 раз каждую ячейку, инвалидируем вначале все ячейки
	// а вовремя пересчета будет делать их валидными и проверять чтобы не пересчитывать валидные ячейки
	ReEvaluate();
}

Cell::~Cell() {}

void Cell::ReEvaluateReferringCells() {
	for (const auto& pos : referring_cells_) {
		auto temp_cell = static_cast<Cell*>(sheet_.GetCell(pos));
		if (!temp_cell->IsValid()) {
			temp_cell->ReEvaluate();
		}
	}
}

void Cell::ReEvaluate() {
	// Если это формульная ячейка, пересчитываем её, а затем
	// пересчитываем все ячейки, которые зависят от текущей
	if (IsFormulaImpl()) {
		static_cast<CellImpl::FormulaImpl*>(impl_.get())->Evaluate(sheet_);		
	}
	InvalidateReferringCells();
	ReEvaluateReferringCells();	
}

bool Cell::IsFormulaImpl() {
	return dynamic_cast<CellImpl::FormulaImpl*>(impl_.get());
}

void Cell::Invalidate() {
	if (IsFormulaImpl()) {
		static_cast<CellImpl::FormulaImpl*>(impl_.get())->Invalidate();
	}	
}

void Cell::InvalidateReferringCells() {
	for (const auto& pos : referring_cells_) {		
		static_cast<Cell*>(sheet_.GetCell(pos))->Invalidate();
	}
}

void Cell::CreateImpl(std::string_view text) {
	// Пустая ячейка
	if (text.empty()) {
		impl_ = std::make_unique<CellImpl::EmptyImpl>();
	}
	// Формула
	if (text.size() > 1 && text[0] == FORMULA_SIGN) {
		impl_ = std::make_unique<CellImpl::FormulaImpl>(text, sheet_, self_);
	}
	// Текстовая
	else {
		impl_ = std::make_unique<CellImpl::TextImpl>(text);
	}
}

bool Cell::IsValid() {
	// Если это формульная ячейка и она валидна, текстовая ячейка не может быть невалидной
	if (IsFormulaImpl()) {
		return static_cast<CellImpl::FormulaImpl*>(impl_.get())->IsValid();
	}
	else {
		return true;
	}
}

std::vector<Position>& Cell::GetReferringCells() {
	return referring_cells_;
}

void Cell::Clear() {
	CreateImpl("");
}

CellInterface::Value Cell::GetValue() const {
	return impl_->GetValue();
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();		
}