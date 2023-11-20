#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (pos.IsValid()) {
        IncreaseSheetSize(pos);
        auto& new_cell = sheet_[pos.row][pos.col];
        if (!new_cell) {
            new_cell = std::make_unique<Cell>(*this);
        }
        new_cell->Set(std::move(text));
    } else {
        throw InvalidPositionException("SetCell method: Position is out of sheet range");
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (pos.IsValid()) {
        if (IsActualPosition(pos)) {
            return sheet_[pos.row][pos.col].get();
        } else {
            return nullptr;
        }
    } else {
        throw InvalidPositionException("GetCell method: Position is out of sheet range");
    }
}

CellInterface* Sheet::GetCell(Position pos) {
    if (pos.IsValid()) {
        if (IsActualPosition(pos)) {
            return sheet_[pos.row][pos.col].get();
        } else {
            return nullptr;
        }
    } else {
        throw InvalidPositionException("GetCell method: Position is out of sheet range");
    }
}

void Sheet::ClearCell(Position pos) {
    if (pos.IsValid()) {
        if (IsActualPosition(pos)) {
            auto& current_cell = sheet_[pos.row][pos.col];
            if (current_cell) {
                current_cell.reset();
            }
        }
    } else {
        throw InvalidPositionException("ClearCell method: Position is out of sheet range");
    }
}

Size Sheet::GetPrintableSize() const {
    Size print_size = {0, 0};
    for (int row = static_cast<int>(sheet_.size()) - 1; row >= 0; --row) {
        for (int col = static_cast<int>(sheet_[row].size()) - 1; col >= 0; --col) {
            if (sheet_[row][col] && !sheet_[row][col]->GetText().empty()) {
                print_size.rows = std::max(print_size.rows, row + 1);
                print_size.cols = std::max(print_size.cols, col + 1);
            }
        }
    }
    return print_size;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int r = 0; r < GetPrintableSize().rows; ++r) {
        for (int c = 0; c < GetPrintableSize().cols; ++c) {
            if (c > 0) {
                output << '\t';
            }
            auto& cell = sheet_[r][c];
            if (c < static_cast<int>(sheet_[r].size()) && cell) {
                std::visit([&output](const auto& item){
                    output << item;
                    }, cell->GetValue()
                );
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int r = 0; r < GetPrintableSize().rows; ++r) {
        for (int c = 0; c < GetPrintableSize().cols; ++c) {
            if (c > 0) {
                output << '\t';
            }
            auto& cell = sheet_[r][c];
            if (c < static_cast<int>(sheet_[r].size()) && cell) {
                output << cell->GetText();
            }
        }
        output << '\n';
    }
}

void Sheet::IncreaseSheetSize(Position pos) {
    int current_rows_count = static_cast<int>(sheet_.size());
    if (current_rows_count < pos.row + 1) {
        sheet_.resize(pos.row + 1);
        int current_cols_count = static_cast<int>(sheet_[pos.row].size());
        for (int i = 0; i < (pos.row + 1 - current_rows_count); ++i) {
            std::vector<std::unique_ptr<Cell>> new_row(current_cols_count);
            for (int j = 0; j <= current_cols_count; ++j) {
                new_row.push_back(std::make_unique<Cell>(*this));
            }
            sheet_.push_back(std::move(new_row));
        }
    }
    if (static_cast<int>(sheet_[pos.row].size()) < pos.col + 1) {
        sheet_[pos.row].resize(pos.col + 1);
        for (auto& row : sheet_) {
            row.push_back(std::make_unique<Cell>(*this));
        }
    }
}

bool Sheet::IsActualPosition(Position pos) const {
    if (pos.IsValid()) {
        if (pos.row >= 0 && pos.row < static_cast<int>(sheet_.size())) {
            if (pos.col >= 0 && pos.col < static_cast<int>(sheet_[pos.row].size())) {
                return true;
            }
        }
    }
    return false;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}