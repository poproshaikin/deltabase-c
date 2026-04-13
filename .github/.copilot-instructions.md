# Copilot Instructions

## General Rules

- Write all code comments strictly in English.
- Avoid any unnecessary debug output (`printf`, `std::cout`, logging, etc.) unless explicitly required.
- Do not execute arbitrary commands.
    - The only allowed implicit command is:
      ```bash
      cmake --build .
      ```
    - Any other command must be executed **only if explicitly instructed**.
- Do not introduce temporary or "quick-fix" solutions that degrade code quality.

---

## Code Organization

- All implementations of **domain (logical) classes** must be placed in corresponding `.cpp` files.
    - Headers (`.h` / `.hpp`) must contain only declarations.
- DTOs (Data Transfer Objects) may be header-only if trivial.
- Maintain clear separation of concerns:
    - Parsing, logic, storage, networking, etc. must not be mixed.

---

## Contracts and Correctness

- If a method has an incorrect or incomplete contract:
    - Fix the **signature**, **implementation**, and **all call sites** in the same change.
- Never leave:
    - partially implemented functions
    - stubbed logic that breaks invariants
    - incorrect return types just to "make it compile"
- Prefer correctness over minimal diff.

---

## Refactoring Rules

- Do not avoid refactoring when it is required to fix design issues.
- If a change breaks an API:
    - propagate the fix across the entire codebase.
- Do not introduce duplication to avoid touching existing code.

---

## Clean Code Principles (Mandatory)

Follow these principles strictly:

### Naming

- Use descriptive and unambiguous names.
- Avoid abbreviations unless they are standard (`id`, `url`, etc.).
- Class names → nouns
- Function names → verbs
- Boolean variables → `is`, `has`, `can`, etc.

### Functions

- Functions must:
    - do one thing
    - be small
    - have a clear purpose
- Avoid side effects unless explicitly required.
- Prefer early returns over deep nesting.

### Readability

- Code must be self-explanatory.
- Comments should explain **why**, not **what**.
- Avoid redundant comments.

### Error Handling

- Do not ignore errors.
- Handle them explicitly or propagate them properly.
- Avoid hidden failure states.

### DRY (Don't Repeat Yourself)

- Eliminate duplication.
- Extract reusable logic into functions or classes.

### SOLID Principles

- **S** — Single Responsibility Principle
- **O** — Open/Closed Principle
- **L** — Liskov Substitution Principle
- **I** — Interface Segregation Principle
- **D** — Dependency Inversion Principle

### Dependencies

- Depend on abstractions, not concrete implementations.
- Avoid tight coupling.

### Data vs Behavior

- Do not mix pure data structures with business logic.
- Domain objects must encapsulate behavior.

---

## C++ Specific Guidelines

- Prefer `std::unique_ptr` over raw pointers.
- Avoid manual memory management unless absolutely necessary.
- Use `const` correctness everywhere applicable.
- Prefer `std::string` over C-style strings.
- Use RAII for resource management.
- Avoid undefined behavior at all costs.

---

## clang-format (Mandatory)

- All code must conform to project `clang-format` configuration.
- Do not manually format code in a conflicting way.
- Ensure:
    - consistent indentation
    - proper spacing
    - aligned braces and declarations
- Formatting is not optional.

---

## clang-tidy (Mandatory)

- Code must pass `clang-tidy` checks.
- Fix warnings instead of suppressing them whenever possible.
- Pay special attention to:
    - memory safety
    - modern C++ usage
    - performance issues
    - readability warnings
- Do not introduce new warnings.

---

## Prohibited Practices

- No commented-out code blocks.
- No dead code.
- No magic numbers (use named constants).
- No hidden side effects.
- No global mutable state unless explicitly required.

---

## Final Rule

- Every change must leave the codebase in a **better state**:
    - more correct
    - more readable
    - more maintainable