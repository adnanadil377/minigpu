#pragma once

#include <string>
#include <memory>
#include <utility>
#include <cstdint>
#include <iostream>

namespace minigpu {

/// Source location tracking line and column in a file.
struct SourceLocation {
    std::string filename;
    int line{1};
    int column{1};

    SourceLocation() = default;
    SourceLocation(std::string fn, int l, int c)
        : filename(std::move(fn)), line(l), column(c) {}

    std::string to_string() const {
        return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
};

/// Type kinds supported by MiniGPU.
enum class TypeKind {
    Void,
    Int,    // 32-bit signed integer
    Float,  // 32-bit single-precision float
    Bool,   // 1-bit boolean
    Pointer // Pointer to element type (e.g. float*, int*)
};

/// Representation of a MiniGPU type.
class Type {
public:
    explicit Type(TypeKind kind, std::shared_ptr<Type> elem_type = nullptr)
        : kind_(kind), elem_type_(std::move(elem_type)) {}

    TypeKind kind() const { return kind_; }
    const std::shared_ptr<Type>& element_type() const { return elem_type_; }

    bool is_void() const { return kind_ == TypeKind::Void; }
    bool is_int() const { return kind_ == TypeKind::Int; }
    bool is_float() const { return kind_ == TypeKind::Float; }
    bool is_bool() const { return kind_ == TypeKind::Bool; }
    bool is_pointer() const { return kind_ == TypeKind::Pointer; }
    bool is_numeric() const { return is_int() || is_float(); }

    size_t size_bytes() const {
        switch (kind_) {
            case TypeKind::Void: return 0;
            case TypeKind::Bool: return 1;
            case TypeKind::Int: return 4;
            case TypeKind::Float: return 4;
            case TypeKind::Pointer: return 8; // 64-bit addresses on GPU
        }
        return 0;
    }

    std::string to_string() const {
        switch (kind_) {
            case TypeKind::Void: return "void";
            case TypeKind::Int: return "int";
            case TypeKind::Float: return "float";
            case TypeKind::Bool: return "bool";
            case TypeKind::Pointer:
                return (elem_type_ ? elem_type_->to_string() : "void") + "*";
        }
        return "unknown";
    }

    bool equals(const Type& other) const {
        if (kind_ != other.kind_) return false;
        if (kind_ == TypeKind::Pointer) {
            if (!elem_type_ || !other.elem_type_) return elem_type_ == other.elem_type_;
            return elem_type_->equals(*other.elem_type_);
        }
        return true;
    }

    // Static factory methods for shared instances
    static std::shared_ptr<Type> getVoid() {
        static auto t = std::make_shared<Type>(TypeKind::Void);
        return t;
    }
    static std::shared_ptr<Type> getInt() {
        static auto t = std::make_shared<Type>(TypeKind::Int);
        return t;
    }
    static std::shared_ptr<Type> getFloat() {
        static auto t = std::make_shared<Type>(TypeKind::Float);
        return t;
    }
    static std::shared_ptr<Type> getBool() {
        static auto t = std::make_shared<Type>(TypeKind::Bool);
        return t;
    }
    static std::shared_ptr<Type> getPointer(std::shared_ptr<Type> elem) {
        return std::make_shared<Type>(TypeKind::Pointer, std::move(elem));
    }

private:
    TypeKind kind_{TypeKind::Void};
    std::shared_ptr<Type> elem_type_{nullptr};
};

inline bool operator==(const Type& lhs, const Type& rhs) {
    return lhs.equals(rhs);
}

inline bool operator!=(const Type& lhs, const Type& rhs) {
    return !lhs.equals(rhs);
}

} // namespace minigpu
