pytest is a mature, full-featured Python testing framework that makes it easy to write simple tests while scaling to support complex functional testing. It features auto-discovery of test modules and functions, detailed assertion introspection (no need to remember `self.assert*` names), modular fixtures for managing test resources, and a rich plugin architecture with over 1300+ external plugins.

**When to use pytest:**
- Writing unit tests, integration tests, or functional tests
- Testing Python applications and libraries
- When you need detailed failure reports with assertion introspection
- When you want to run unittest or trial test suites out of the box

**When not to use pytest:**
- For non-Python projects (use language-specific frameworks)
- When you need a GUI test runner (use IDE-specific tools)
- For extremely simple scripts where `unittest` is sufficient

**Key design principles:**
- Simple `assert` statements with detailed introspection
- Fixture-based dependency injection
- Plugin architecture for extensibility
- Auto-discovery of tests following naming conventions

**Installation:**
```bash
pip install pytest
# For additional features:
pip install pytest-cov pytest-xdist pytest-mock
```
