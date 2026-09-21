// Deliberate syntax errors (missing semicolons) inside a function body,
// to demonstrate panic-mode error recovery: the parser should report
// both errors and keep parsing the rest of the file.
int compute(int a, int b) {
    int total = a + b
    int doubled = total * 2;
    print(doubled)
    return doubled;
}

int after = 1;
