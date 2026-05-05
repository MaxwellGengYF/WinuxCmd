**Red-line condition 1: Never use bare `except:` in test code**
```python
# BAD: Catches all exceptions, including KeyboardInterrupt
def test_something():
    try:
        result = risky_operation()
    except:
        assert False

# GOOD: Catch specific exceptions
def test_something():
    try:
        result = risky_operation()
    except ValueError as e:
        pytest.fail(f"ValueError occurred: {e}")
```

**Red-line condition 2: Never modify global state in tests**
```python
# BAD: Modifies global variable
GLOBAL_CONFIG = {"debug": False}

def test_config():
    GLOBAL_CONFIG["debug"] = True
    assert GLOBAL_CONFIG["debug"] == True

# GOOD: Use fixture with teardown
@pytest.fixture
def config():
    original = GLOBAL_CONFIG.copy()
    GLOBAL_CONFIG["debug"] = True
    yield
    GLOBAL_CONFIG.clear()
    GLOBAL_CONFIG.update(original)
```

**Red-line condition 3: Never use `assert False` without context**
```python
# BAD: Unclear failure reason
def test_condition():
    if not some_condition():
        assert False

# GOOD: Provide meaningful message
def test_condition():
    assert some_condition(), "some_condition() returned False"
```

**Red-line condition 4: Never ignore test failures silently**
```python
# BAD: Silently passes
def test_ignored():
    try:
        assert 1 == 2
    except AssertionError:
        pass  # Test passes even though assertion failed

# GOOD: Let assertion fail naturally
def test_proper():
    assert 1 == 2  # This will properly fail
```

**Red-line condition 5: Never use `pytest.skip()` without condition**
```python
# BAD: Skips test unconditionally
def test_skipped():
    pytest.skip("Skipping this test")

# GOOD: Skip conditionally
import sys
def test_skipped():
    if sys.version_info < (3, 8):
        pytest.skip("Requires Python 3.8+")
    assert True
```
