# NovaC

NovaC is a modular C++ framework for building programming languages.

Instead of hardcoding keywords, operators, functions, control flow, or runtime behavior directly into the engine, NovaC lets you compose the features you want and use them to build your own language.

## Getting Started

Clone the repository:

```bash
git clone https://github.com/GizmekOrochi/NovaC.git
cd NovaC
```

Build the documentation:

```bash
make doc
```

The documentation contains explanations of the architecture, API usage, customization, and several examples.

You can also run the test suite with:

```bash
make test
```

NovaC currently has more than 500 automated tests covering the lexer, parser, AST, runtime, registries, features, HIR, MIR, and other core components.

## Project Status

NovaC is still a work in progress.

Building it taught me a lot about compilers, interpreters, language design, testing, and software architecture.

I am sure that not every design decision is perfect, but experimenting with the architecture and trying to make the different parts work together has been one of the most interesting parts of the project.

After roughly six months of working on NovaC, I am currently taking a break from the project.

NovaC is not abandoned. There are still ideas, improvements, and features I would like to explore when I come back to it.

## Documentation

The full documentation can be generated locally with:

```bash
make doc
```

Examples are included in the documentation and in the repository.

## License

NovaC is released under the MIT License.

If you use NovaC as part of your own language or project, a mention is appreciated.
