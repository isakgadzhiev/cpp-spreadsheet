#include "cell.h"

#include <string>
#include <optional>


// class Cell
Cell::Cell(SheetInterface& sheet)
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet)
    , cache_(std::nullopt) {
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    if (text.size() > 1 && text[0] == FORMULA_SIGN ) {
        auto temp = std::make_unique<FormulaImpl>(ParseFormula(text.substr(1)), sheet_);
        if (CheckCircle(temp)) {
            throw CircularDependencyException("Error. Cell has a cyclic formula!");
        }
        if (impl_) {
            DeleteInverseDependenciesAndCache();
        }
        impl_ = std::move(temp);
        cache_ = impl_->GetValue();
        AddInverseDependencies();
    } else if (!text.empty()) {
        impl_ = std::make_unique<TextImpl>(text);
        cache_ = impl_->GetValue();
    } else {
        impl_ = std::make_unique<EmptyImpl>();
    }
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    if (!cache_) {
        cache_ = impl_->GetValue();
    }
    return cache_.value();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

void Cell::AddInverseDependencies() {
    for (const auto& cell : impl_->GetReferencedCells()) {
        CellInterface* cell_ptr = sheet_.GetCell(cell);
        if (cell_ptr) {
            static_cast<Cell*>(cell_ptr)->influencing_cells_.insert(this);
        }
    }
}

void Cell::DeleteInverseDependenciesAndCache() {
    for (const auto& cell : impl_->GetReferencedCells()) {
        CellInterface* cell_ptr = sheet_.GetCell(cell);
        if (cell_ptr) {
            dynamic_cast<Cell*>(cell_ptr)->cache_ = std::nullopt;
            dynamic_cast<Cell*>(cell_ptr)->influencing_cells_.erase(this);
        }
    }
}

// class Cell::TextImpl
Cell::TextImpl::TextImpl(std::string &text)
        : text_(std::move(text)) {
}

void Cell::TextImpl::Set(std::string text) {
    text_ = std::move(text);
}

std::string Cell::TextImpl::GetText() const {
    return text_;
}

Cell::Value Cell::TextImpl::GetValue() const {
    if (!text_.empty() && text_[0] == ESCAPE_SIGN) {
        return text_.substr(1);
    } else {
        return text_;
    }
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const {
    return {};
}

// class Cell::FormulaImpl
Cell::FormulaImpl::FormulaImpl(std::unique_ptr<FormulaInterface> formula, SheetInterface& sheet)
        : formula_(std::move(formula))
        , sheet_(sheet) {
}

void Cell::FormulaImpl::Set(std::string text) {
    formula_ = ParseFormula(text);
}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_->GetExpression();
}

Cell::Value Cell::FormulaImpl::GetValue() const {
    auto result = formula_->Evaluate(sheet_);
    if (std::holds_alternative<double>(result)) {
        // Если результат - число, вернуть его как double
        return std::get<double>(result);
    } else {
        // Если возникла ошибка, вернуть её как FormulaError
        return std::get<FormulaError>(result);
    }
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}

// class Cell::EmptyImpl
void Cell::EmptyImpl::Set(std::string text) {
    empty_impl_->Set(text);
}

Cell::Value Cell::EmptyImpl::GetValue() const {
    return "";
}

std::string Cell::EmptyImpl::GetText() const {
    return "";
}

std::vector<Position> Cell::EmptyImpl::GetReferencedCells() const {
    return {};
}

bool Cell::HasCircleDependency(const Position& current_cell, std::unordered_set<CellInterface*>& visited_cells) {
    const auto cur_cell_ptr = sheet_.GetCell(current_cell);
    if (cur_cell_ptr) {
        if (cur_cell_ptr == this) {
            return true;
        }
        visited_cells.insert(cur_cell_ptr);
        for (const auto& cell_pos : cur_cell_ptr->GetReferencedCells()) {
            const auto cell_pos_ptr = sheet_.GetCell(cell_pos);
            if (!visited_cells.count(cell_pos_ptr) &&
                HasCircleDependency(cell_pos, visited_cells/*, stack*/)) {
                return true;
            }
            visited_cells.insert(cell_pos_ptr);
        }
    }
    return false;
}

bool Cell::CheckCircle(std::unique_ptr<FormulaImpl>& cell_ptr) {
    const auto& referenced_cells = cell_ptr->GetReferencedCells();
    if (!referenced_cells.empty()) {
        std::unordered_set<CellInterface*> visited_cells;
        for (const auto& cell_pos : referenced_cells) {
            if (HasCircleDependency(cell_pos, visited_cells)) {
                return true;
            }
        }
    }
    return false;
}