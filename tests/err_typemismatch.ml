// Deliberate semantic errors: type mismatches
bool flag = 5;          // cannot initialize bool with int
int total = true;       // cannot initialize int with bool

int add(int a, int b) {
    return a + b;
}

int checker() {
    int x = 10;
    if (x + 1) {         // condition must be bool, not int
        print(x);
    }
    bool r = add(x, true);  // second argument should be int, not bool
    return r;                // returning bool from an int function
}
