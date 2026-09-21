// Type promotion (int -> float) and recursive-style function calls
float average(int a, int b) {
    float total = a + b;
    return total / 2;
}

bool is_positive(float v) {
    return v > 0;
}

float result = average(4, 7);
bool ok = is_positive(result);
