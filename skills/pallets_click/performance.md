Performance characteristics and optimization patterns for Click applications.

**Use lazy imports for heavy dependencies:**
```python
import click

@click.group()
def cli():
    """CLI with lazy loading."""
    pass

@cli.command()
def heavy_task():
    """Task with heavy dependencies."""
    # Import only when command is executed
    import numpy as np
    import pandas as pd
    
    data = pd.DataFrame({'a': [1, 2, 3]})
    result = np.mean(data['a'])
    click.echo(f"Result: {result}")

@cli.command()
def light_task():
    """Lightweight task."""
    click.echo("Quick operation")

if __name__ == '__main__':
    cli()
```

**Profile command execution time:**
```python
import click
import time
from functools import wraps

def timed_command(func):
    """Decorator to time command execution."""
    @wraps(func)
    def wrapper(*args, **kwargs):
        start = time.perf_counter()
        result = func(*args, **kwargs)
        elapsed = time.perf_counter() - start
        click.echo(f"Execution time: {elapsed:.3f}s", err=True)
        return result
    return wrapper

@click.command()
@timed_command
@click.argument('count', type=int)
def process_items(count):
    """Process items with timing."""
    total = 0
    for i in range(count):
        total += i ** 2
    click.echo(f"Sum of squares: {total}")

if __name__ == '__main__':
    process_items()
```

**Optimize option parsing with callbacks:**
```python
import click
import re

# Pre-compile regex for performance
EMAIL_PATTERN = re.compile(r'^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$')

def validate_email(ctx, param, value):
    """Fast email validation with pre-compiled regex."""
    if value is not None and not EMAIL_PATTERN.match(value):
        raise click.BadParameter(f"Invalid email: {value}")
    return value

@click.command()
@click.option('--email', callback=validate_email, help='Email address')
@click.option('--count', default=1000, type=int, help='Number of iterations')
def send_notifications(email, count):
    """Send notifications efficiently."""
    click.echo(f"Sending to {email}")
    # Batch processing for performance
    batch_size = 100
    for i in range(0, count, batch_size):
        batch = range(i, min(i + batch_size, count))
        click.echo(f"Processing batch {i//batch_size + 1}")
        # Process batch...

if __name__ == '__main__':
    send_notifications()
```

**Use caching for repeated computations:**
```python
import click
from functools import lru_cache

@lru_cache(maxsize=128)
def expensive_computation(n):
    """Expensive computation with caching."""
    result = 0
    for i in range(n):
        result += i ** 3
    return result

@click.command()
@click.argument('numbers', nargs=-1, type=int)
def compute(numbers):
    """Compute with caching."""
    for n in numbers:
        result = expensive_computation(n)
        click.echo(f"f({n}) = {result}")

if __name__ == '__main__':
    compute()
```

**Memory-efficient file processing:**
```python
import click

@click.command()
@click.argument('input_file', type=click.File('r'))
@click.argument('output_file', type=click.File('w'))
def process_large_file(input_file, output_file):
    """Process large files line by line."""
    for line in input_file:
        # Process each line without loading entire file
        processed = line.strip().upper()
        output_file.write(processed + '\n')
    
    click.echo("Processing complete")

if __name__ == '__main__':
    process_large_file()
```
