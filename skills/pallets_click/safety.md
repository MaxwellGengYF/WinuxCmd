Five conditions that must NEVER occur when using Click.

**1. Never expose sensitive data in help text:**
```python
# BAD - password in help text
import click

@click.command()
@click.option('--password', help='Your password: admin123')
def login(password):
    click.echo("Logged in")
```
```python
# GOOD - use hidden_input and generic help
import click

@click.command()
@click.option('--password', prompt=True, hide_input=True, help='Your password')
def login(password):
    click.echo("Logged in")
```

**2. Never use eval or exec with user input:**
```python
# BAD - dangerous eval
import click

@click.command()
@click.argument('expression')
def calculate(expression):
    result = eval(expression)  # SECURITY RISK
    click.echo(result)
```
```python
# GOOD - safe evaluation
import click
import ast

@click.command()
@click.argument('expression')
def calculate(expression):
    try:
        tree = ast.parse(expression, mode='eval')
        # Only allow safe operations
        result = eval(compile(tree, '<string>', 'eval'),
                     {"__builtins__": {}}, {})
    except Exception as e:
        click.echo(f"Error: {e}", err=True)
        raise click.Abort()
```

**3. Never ignore file path injection:**
```python
# BAD - direct path usage
import click

@click.command()
@click.argument('path')
def delete(path):
    import os
    os.remove(path)  # Can delete any file
```
```python
# GOOD - restrict to safe directory
import click
import os

SAFE_DIR = '/home/user/data'

@click.command()
@click.argument('filename')
def delete(filename):
    safe_path = os.path.normpath(os.path.join(SAFE_DIR, filename))
    if not safe_path.startswith(SAFE_DIR):
        click.echo("Invalid path", err=True)
        raise click.Abort()
    os.remove(safe_path)
```

**4. Never use shell=True with user input:**
```python
# BAD - shell injection risk
import click
import subprocess

@click.command()
@click.argument('command')
def run(command):
    subprocess.run(command, shell=True)  # SECURITY RISK
```
```python
# GOOD - use list form
import click
import subprocess

@click.command()
@click.argument('command')
@click.argument('args', nargs=-1)
def run(command, args):
    subprocess.run([command] + list(args))
```

**5. Never expose internal errors to users:**
```python
# BAD - shows traceback
import click

@click.command()
@click.argument('number', type=int)
def divide(number):
    result = 100 / number  # ZeroDivisionError shows traceback
    click.echo(result)
```
```python
# GOOD - handle errors gracefully
import click

@click.command()
@click.argument('number', type=int)
def divide(number):
    try:
        result = 100 / number
    except ZeroDivisionError:
        click.echo("Cannot divide by zero", err=True)
        raise click.Abort()
    click.echo(result)
```
