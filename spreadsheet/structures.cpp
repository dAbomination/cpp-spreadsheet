#include "common.h"

#include <cctype>
#include <sstream>

FormulaError::FormulaError(Category category)
	: category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
	return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
	return this->category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
	switch (category_) {
	case Category::Div0:
		return "#DIV/0!";
	case Category::Value:
		return "#VALUE!";
	case Category::Ref:
		return "#REF!";
	default:
		return "#UNKNOWN_ERROR";
	}
}

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 8;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = {-1, -1};

bool Position::operator==(const Position rhs) const {
	return (this->col == rhs.col) && (this->row == rhs.row);
}

bool Position::operator<(const Position rhs) const {
	return (this->col < rhs.col) || (this->row < rhs.row);
}

bool Position::IsValid() const {
	return (row < MAX_ROWS) && (col < MAX_COLS) && (row >= 0) && (col >= 0);
}

std::string Position::ToString() const {
	if (!IsValid()) {
		return "";
	}
	std::string result;
	// Так как в программе нумерация с нуля, а у пользователя с единицы, прибавляем 1 к номерам столбца и строки
	// Номер столбца	
	int temp_col = col + 1;

	while (temp_col > 0) {		
		int index = (temp_col - 1) % 26;
		result = char(index + 'A') + std::move(result);
		temp_col = (temp_col - 1) / 26;
	}
	// Номер строки
	result += std::to_string(row + 1);

	return result;
}

int ColPosStringToInt(std::string_view str) {
	int result = 0;

	for (const char& c : str)
	{
		result *= 26;
		result += c - 'A' + 1;
	}
	
	return result;
}

Position Position::FromString(std::string_view str) {
	if (str.length() > MAX_POSITION_LENGTH || str.empty()) {
		return Position::NONE;
	}
	
	std::string col_str, row_string;

	size_t str_size = str.size();
	bool char_passed = false;
	for (size_t char_num = 0; char_num < str_size; ++char_num) {
		// Если это не цифра или буква(заглавная), значит позиция невалидна
		if (!std::isalnum(str[char_num]) || std::islower(str[char_num])) {
			return Position::NONE;
		}
		else if (std::isalpha(str[char_num]) && std::isupper(str[char_num])) {
			if (char_passed) {
				return Position::NONE;
			}
			col_str += str[char_num];
		}
		else {
			char_passed = true;
			row_string += str[char_num];
		}
	}

	if (col_str.empty() || row_string.empty()) {
		return Position::NONE;
	}
	// Аналогично, необходимо вычесть 1 т.к. нумерация в системе начинается с нуля, а у пользователя с 1
	return { std::stoi(row_string) - 1, ColPosStringToInt(col_str) - 1 };
}

bool Size::operator==(Size rhs) const {
	return (this->cols == rhs.cols) && (this->rows == rhs.rows);
}
