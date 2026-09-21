// Deliberate semantic errors: undeclared identifiers
int compute() {
    int result = a + b;   // 'a' and 'b' were never declared
    print(result);
    return unknown_func(result);  // function never declared
}
