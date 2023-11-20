#pragma once

#include <optional>
#include <functional>

#include "common.h"
#include "formula.h"

class Cell : public CellInterface {
public:
    Cell(const SheetInterface& sheet);
    ~Cell();
    void Set(std::string text);
    void Clear();
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    void AddDependencies(std::vector<Position>& referenced_cells);
    bool IsReferenced() const;
    bool CheckCircle();


private:
    class Impl {
    public:
        virtual ~Impl() = default;
        virtual void Set(std::string text) = 0;
        virtual CellInterface::Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;
    };

    class TextImpl : public Impl {
    public:
        explicit TextImpl(std::string& text);
        void Set(std::string text) override;
        std::string GetText() const override;
        Value GetValue() const override;
        std::vector<Position> GetReferencedCells() const override;

    private:
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:
        explicit FormulaImpl(std::unique_ptr<FormulaInterface> formula);
        void Set(std::string text) override;
        std::string GetText() const override;
        Value GetValue() const override;
        std::vector<Position> GetReferencedCells() const override;

    private:
        std::unique_ptr<FormulaInterface> formula_;
    };

    class EmptyImpl : public Impl {
    public:
        EmptyImpl() = default;
        void Set(std::string text) override;
        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;

    private:
        std::unique_ptr<Impl> empty_impl_;
    };

    std::unique_ptr<Impl> impl_;
    const SheetInterface& sheet_;
    bool have_referenced_cells_ = false;
    std::vector<Position> depend_cells_;
    mutable std::optional<double> cache_ = std::nullopt;
};