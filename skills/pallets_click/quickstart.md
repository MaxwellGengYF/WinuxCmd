Here are common patterns for building CLI tools with Click. Install with `pip install click`.

**Basic command with options:**
```python
import click

@click.command()
@click.option('--name', prompt='Your name', help='The person to greet.')
@click.option('--count', default=1, help='Number of greetings.')
def hello(name, count):
    """Simple program that greets NAME for a total of COUNT times."""
    for _ in range(count):
        click.echo(f"Hello, {name}!")

if __name__ == '__main__':
    hello()
```

**Command with arguments:**
```python
import click

@click.command()
@click.argument('filename')
@click.option('--verbose', '-v', is_flag=True, help='Enable verbose output.')
def process(filename, verbose):
    """Process a file."""
    if verbose:
        click.echo(f"Processing {filename}...")
    click.echo(f"Done with {filename}")

if __name__ == '__main__':
    process()
```

**Nested command groups:**
```python
import click

@click.group()
def cli():
    """A CLI with subcommands."""
    pass

@cli.command()
@click.argument('name')
def greet(name):
    """Greet someone."""
    click.echo(f"Hello, {name}!")

@cli.command()
@click.argument('name')
def farewell(name):
    """Say goodbye."""
    click.echo(f"Goodbye, {name}!")

if __name__ == '__main__':
    cli()
```

**Using context for shared state:**
```python
import click

@click.group()
@click.option('--debug/--no-debug', default=False)
@click.pass_context
def cli(ctx, debug):
    """CLI with shared debug flag."""
    ctx.ensure_object(dict)
    ctx.obj['DEBUG'] = debug

@cli.command()
@click.pass_context
def status(ctx):
    """Show status."""
    if ctx.obj['DEBUG']:
        click.echo("Debug mode is ON")
    click.echo("Status: OK")

if __name__ == '__main__':
    cli()
```

**Choice options and passwords:**
```python
import click

@click.command()
@click.option('--mode', type=click.Choice(['read', 'write', 'execute'], case_sensitive=False))
@click.option('--password', prompt=True, hide_input=True, confirmation_prompt=True)
def configure(mode, password):
    """Configure with mode and password."""
    click.echo(f"Mode: {mode}")
    click.echo(f"Password set: {'*' * len(password)}")

if __name__ == '__main__':
    configure()
```

**File input/output:**
```python
import click

@click.command()
@click.argument('input', type=click.File('r'))
@click.argument('output', type=click.File('w'))
def copy(input, output):
    """Copy input file to output file."""
    output.write(input.read())
    click.echo("Copy complete")

if __name__ == '__main__':
    copy()
```
