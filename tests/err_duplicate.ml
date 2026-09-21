// Deliberate semantic errors: duplicate declarations
int total = 5;
int total = 10;     // redeclared in global scope

int add(int a, int a) {   // duplicate parameter name
    return a;
}

int add(int x, int y) {   // function 'add' already declared
    return x + y;
}
