# molecular-programgenerator

Generates complete shader programs from code snippets. You provide it with a
set of input variables (e.g. vertex position, transform matrix and color), and
one or more requested outputs (like vertex position and fragment color). It
then attempts to create a tree out of the given code snippets that calculate the
output variables from the input variables.

## Snippet Files

Snippet files contain a series of functions, with each function having the
following syntax.

```
digit = '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9' ;
character = 'a' | 'b' ... 'z' | 'A' | 'B' ... 'Z' ;
number = [ '-' ], digit, { digit } ;
identifier = character, { character | digit } ;
parameter = [whitespace], identifier, whitespace, identifier, [whitespace] ;
attribute = 'fragment' | 'vertex' | 'low_q' | 'prio=', number ;
body = '{', ?text with balanced parantheses?, '}' ;
function = [whitespace], {attribute, whitespace}, identifier, whitespace, identifier, [whitespace], '(', [parameter, {',', parameter}], ')', [whitespace], body ;
```

## Usage

### Step 1: Load Snippet Files

Load text files with `ProgramFile`:
```
FILE* file = fopen("file1.txt", "r");
char buffer[4096]; // Assume files are not larger than 4K for simplicity
size_t size = fread(buffer, 1, 4096, file);

ProgramFile programFile1(buffer, buffer + size);
fclose(file);
```

### Step 2: Feed File Contents Into Generator

```
ProgramGenerator generator;

for(auto& variable: programFile1.GetVariables())
    generator.AddVariable(variable);

for(auto& function: programFile1.GetFunctions())
    generator.AddFunction(function);
```

### Step 3: Generate Programs

```
// Array containing both inputs and outputs:
std::array<Hash, 4> vars = {"positionAttribute"_H, "modelViewProjectionMatrix"_H, "gl_Position"_H, "fragmentColor"_H};
ProgramText program = generator.GenerateProgram(vars.begin(); vars.end());

myRenderer.CompileProgram(program.vertexShader, program.fragmentShader);
```
