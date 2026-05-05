**Performance characteristics:**
pytest is designed for speed with features like test collection caching and parallel execution. Key performance considerations include fixture scope, parametrization overhead, and test isolation.

```python
# test_performance.py
import pytest
import time

# BAD: Function-scoped fixture for expensive setup
@pytest.fixture
def expensive_resource():
    time.sleep(2)  # Simulate expensive setup
    return {"data": "expensive"}

# GOOD: Session-scoped fixture for expensive setup
@pytest.fixture(scope="session")
def efficient_resource():
    time.sleep(2)  # Only done once
    return {"data": "efficient"}
```

**Profiling tests:**
```bash
# Install profiling tools
pip install pytest-benchmark pytest-profiling

# Profile test execution
pytest --profile test_profile.py

# Benchmark specific functions
pytest --benchmark-only test_benchmark.py
```

**Using pytest-benchmark for performance testing:**
```python
# test_benchmark.py
import pytest

def test_sort_performance(benchmark):
    data = [3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5]
    result = benchmark(sorted, data)
    assert result == [1, 1, 2, 3, 3, 4, 5, 5, 5, 6, 9]

@pytest.mark.parametrize("n", [10, 100, 1000])
def test_list_comprehension_performance(benchmark, n):
    result = benchmark([x*2 for x in range(n)])
    assert len(result) == n
```

**Optimizing parametrized tests:**
```python
# test_param_perf.py
import pytest

# BAD: Creates many test cases
@pytest.mark.parametrize("i", range(10000))
def test_many_cases(i):
    assert i >= 0

# GOOD: Test in batches
def test_batch():
    for i in range(10000):
        assert i >= 0
```
