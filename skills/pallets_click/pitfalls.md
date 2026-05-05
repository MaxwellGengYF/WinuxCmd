Common mistakes when using Click and how to avoid them.

**1. Forgetting to call the function at the end:**
```python
# BAD - missing if __name__ block
import click

@click.command()
@click.option('--name')
def hello(name):
    click.echo(f"Hello {name}")

# This won't work when imported
hello()
```
```python
# GOOD - proper entry point
import click

@click.command()
@click.option('--name')
def hello(name):
    click.echo(f"Hello {name}")

if __name__ == '__main__':
    hello()
```

**2. Confusing arguments and options:**
```python
# BAD - using option for required positional argument
import click

@click.command()
@click.option('--filename', required=True)
def process(filename):
    click.echo(f"Processing {filename}")
```
```python
# GOOD - use argument for required positional values
import click

@click.command()
@click.argument('filename')
def process(filename):
    click.echo(f"Processing {filename}")
```

**3. Not using context for shared state:**
```python
# BAD - global variables
import click

debug = False

@click.command()
@click.option('--debug', is_flag=True)
def set_debug(debug):
    global debug
    debug = debug

@click.command()
def status():
    global debug
    if debug:
        click.echo("Debug mode")
```
```python
# GOOD - use context object
import click

@click.group()
@click.option('--debug', is_flag=True)
@click.pass_context
def cli(ctx, debug):
    ctx.ensure_object(dict)
    ctx.obj['DEBUG'] = debug

@cli.command()
@click.pass_context
def status(ctx):
    if ctx.obj['DEBUG']:
        click.echo("Debug mode")
```

**4. Forgetting to handle file encoding:**
```python
# BAD - assumes UTF-8
import click

@click.command()
@click.argument('file', type=click.File('r'))
def read_file(file):
    content = file.read()
```
```python
# GOOD - explicit encoding
import click

@click.command()
@click.argument('file', type=click.File('r', encoding='utf-8'))
def read_file(file):
    content = file.read()
```

**5. Not using click.echo for output:**
```python
# BAD - using print
import click

@click.command()
def hello():
    print("Hello, World!")
```
```python
# GOOD - use click.echo
import click

@click.command()
def hello():
    click.echo("Hello, World!")
```

**6. Ignoring return values from callbacks:**
```python
# BAD - callback doesn't return value
import click

def validate_count(ctx, param, value):
    if value < 0:
        raise click.BadParameter("Count must be positive")

@click.command()
@click.option('--count', callback=validate_count, type=int)
def repeat(count):
    for _ in range(count):
        click.echo("Hello")
```
```python
# GOOD - callback returns validated value
import click

def validate_count(ctx, param, value):
    if value is not None and value < 0:
        raise click.BadParameter("Count must be positive")
    return value

@click.command()
@click.option('--count', callback=validate_count, type=int)
def repeat(count):
    for _ in range(count):
        click.echo("Hello")
```
