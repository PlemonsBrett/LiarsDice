# Virtual Constexpr Functions in C++23: Technical Limitations and Design Decisions

## Executive Summary

While C++20 introduced `constexpr virtual` functions, their practical application in our dependency injection architecture reveals significant limitations that outweigh the benefits. This document explains why we chose not to use `constexpr virtual` functions in our LiarsDice project interfaces.

## C++23 Virtual Constexpr Overview

### What's Allowed

```cpp
// This is legal C++20/23 syntax
class Base {
public:
    virtual constexpr int getValue() const = 0;
};

class Derived : public Base {
public:
    constexpr int getValue() const override { return 42; }
};
```

### Key Constraints

1. **Constant evaluation context**: Virtual function calls can only be `constexpr` if the dynamic type is known at compile time
2. **Runtime polymorphism conflict**: The fundamental purpose of virtual functions (runtime dispatch) conflicts with `constexpr` requirements (compile-time evaluation)
3. **Limited practical utility**: Most real-world polymorphic scenarios involve runtime type determination

## Technical Analysis

### 1. Compile-Time Type Resolution Requirement

```cpp
// This works - type known at compile time
constexpr auto test() {
    Derived d;
    Base& b = d;  // Static type known
    return b.getValue();  // Can be constexpr
}

// This doesn't work - runtime polymorphism
auto runtime_test(std::unique_ptr<Base> ptr) {
    return ptr->getValue();  // Cannot be constexpr - dynamic dispatch
}
```

**Impact on our codebase**: Our dependency injection container creates objects at runtime with types determined by registration, making compile-time evaluation impossible.

### 2. Interface Design Complexity

```cpp
// Our current clean interface
class IDice {
public:
    virtual bool is_valid_face_value(unsigned int value) const = 0;
};

// Constexpr version creates implementation burden
class IDice {
public:
    virtual constexpr bool is_valid_face_value(unsigned int value) const = 0;
    // ^^^ Forces ALL implementations to be constexpr-compatible
};
```

**Problem**: Once an interface method is marked `constexpr virtual`, ALL implementations must support constant evaluation, even when they don't benefit from it.

### 3. Dependency Injection Incompatibility

Our ServiceContainer architecture:

```cpp
template<typename T>
auto resolve() -> std::expected<std::unique_ptr<T>, DIError> {
    // Runtime type resolution
    auto factory = services_.find(typeid(T));
    return std::unique_ptr<T>(static_cast<T*>(factory->create()));
}
```

**Core Issue**: The factory pattern inherently involves runtime type creation, making the objects incompatible with `constexpr` contexts.

### 4. Mock Testing Complications

```cpp
// Our test mock
class MockRandomGenerator : public IRandomGenerator {
    std::vector<int> sequence_;  // Runtime data
public:
    int generate(int min, int max) override {
        return sequence_[index_++];  // Cannot be constexpr
    }
};
```

**Testing Impact**: Test mocks require runtime state modification, which is incompatible with `constexpr` evaluation.

## Industry Standard Considerations

### When Virtual Constexpr Is Appropriate

1. **Mathematical abstractions** with compile-time evaluable implementations
2. **Policy-based design** where compile-time configuration is needed
3. **Template metaprogramming** requiring runtime polymorphism in constexpr contexts

### When to Avoid (Our Use Case)

1. **Dependency injection systems** - runtime type resolution
2. **Factory patterns** - dynamic object creation
3. **Stateful interfaces** - runtime data dependencies
4. **Test frameworks** - mock objects with runtime behavior

## Performance Analysis

### Theoretical Benefits

- Compile-time function resolution
- Potential optimization opportunities
- Reduced runtime overhead

### Practical Reality in Our Codebase

```cpp
// This function call pattern dominates our usage
void gameLogic() {
    auto dice = serviceContainer.resolve<IDice>().value();
    bool valid = dice->is_valid_face_value(3);  // Always runtime dispatch
    //           ^^^ ServiceContainer returns runtime-created objects
}
```

**Result**: 99.9% of our virtual function calls occur at runtime, making `constexpr` benefits negligible.

## Compiler Support and Tooling

### Clang 20.1.8 Observations

- Strict enforcement of constexpr requirements
- Clear error messages when constexpr context violations occur
- Good optimization for non-virtual constexpr functions

### Build System Impact

```cpp
// This causes compilation errors in our DI system
virtual constexpr bool validate() const = 0;
// Error: function never produces a constant expression
```

**Tooling**: Our CI/CD pipeline with clang-tidy flags virtual constexpr as problematic in dependency injection contexts.

## Alternative Solutions

### 1. Static Constexpr Methods

```cpp
class IDice {
public:
    virtual bool is_valid_face_value(unsigned int value) const = 0;
    
    // Compile-time validation available as static method
    static constexpr bool is_valid_face_value_static(unsigned int value) {
        return value >= 1 && value <= 6;
    }
};
```

### 2. Concepts for Compile-Time Constraints

```cpp
template<typename T>
concept ValidDiceValue = requires(T value) {
    requires std::is_convertible_v<T, unsigned int>;
    requires T(1) >= 1 && T(6) <= 6;
};

template<ValidDiceValue T>
constexpr bool validate_dice_value(T value) {
    return value >= 1 && value <= 6;
}
```

### 3. If Consteval for Hybrid Approaches

```cpp
bool IDice::is_valid_face_value(unsigned int value) const {
    if consteval {
        // Compile-time path when possible
        return value >= 1 && value <= 6;
    } else {
        // Runtime path with full validation
        return this->runtime_validate(value);
    }
}
```

## Recommendation

**Decision**: Do not use `virtual constexpr` in interface definitions for this project.

**Rationale**:

1. **Architectural incompatibility** with dependency injection
2. **No measurable performance benefit** in our usage patterns
3. **Increased complexity** without corresponding value
4. **Testing complications** with mock objects
5. **Industry best practice** favors runtime polymorphism for this use case

## Future Considerations

### When to Revisit

- If the codebase shifts toward compile-time configuration
- If C++26 introduces better virtual constexpr semantics
- If performance profiling shows measurable benefits

### Alternative Technologies

- **Concepts** for compile-time interface constraints
- **CRTP (Curiously Recurring Template Pattern)** for static polymorphism
- **std::variant with std::visit** for type-safe runtime polymorphism
- **Policy-based design** for compile-time configuration

## Conclusion

While `virtual constexpr` is a valid C++23 feature, its application in dependency injection architectures creates more problems than it solves. Our decision to use standard virtual functions aligns with industry best practices for this architectural pattern and maintains code clarity, testability, and maintainability.

The performance benefits of `constexpr` are better achieved through other C++23 features like concepts, `if consteval`, and compile-time algorithms rather than forcing runtime polymorphic interfaces into constexpr contexts.

---

**Document Version**: 1.0  
**Last Updated**: 2025-01-20  
**Author**: Brett Plemons
**Review Status**: Technical Decision Document
