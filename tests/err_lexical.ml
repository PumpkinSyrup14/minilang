// Deliberate lexical errors
int a = 5;
int b = 3.4.5;   // malformed number literal
int c = a @ b;   // '@' is not a valid MiniLang character
