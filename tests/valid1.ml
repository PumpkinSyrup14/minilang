// Basic variables, arithmetic, and control flow
int x = 10;
int y = 20;
float ratio = 0.0;
bool flag = true;

int sum(int a, int b) {
    return a + b;
}

int main_logic() {
    int total = sum(x, y);
    ratio = total / 2;

    if (total > 25 && flag) {
        print(total);
    } else {
        print(0);
    }

    int i = 0;
    while (i < 3) {
        print(i);
        i = i + 1;
    }

    return total;
}
