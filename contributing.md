# Contributing

Contributions are welcome — bug reports, feature requests, documentation improvements, and code
changes are all appreciated.

## Reporting Issues

Please [open a GitHub issue](../../issues) and include:
- A short description of the problem or feature request.
- Steps to reproduce (for bugs), or a brief rationale (for features).
- Relevant environment details (OS, compiler version, DCDB/InfluxDB version).

## Submitting a Pull Request

1. Fork the repository and create a branch from `develop`.
2. Make your changes — keep commits focused and the diff easy to review.
3. Ensure the project builds cleanly:
   ```bash
   cmake -S . -B build
   cmake --build build -j
   ```
4. Open a pull request against `develop` with a clear description of what changed and why.

## Code Style

- **C++ standard:** C++17.
- Keep new functions and classes documented with Doxygen-style comments (`@brief`, `@param`,
  `@return`) consistent with the existing source in `src/`.
- Prefer small, focused changes over large refactors in a single PR.

## License

By contributing, you agree that your contributions will be licensed under the
[Apache License 2.0 with LLVM Exceptions](LICENSE), the same license as this project.
