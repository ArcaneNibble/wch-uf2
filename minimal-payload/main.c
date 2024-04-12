int test = 0x12345;
int test2;

#pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
__attribute__((constructor(0))) static void setup_qingke() {
    asm volatile(
        // Undocumented "Configure pipelining and instruction prediction"
        "li t0, 0x1f\n"
        "csrw 0xbc0, t0\n"
    );
}

__attribute__((constructor)) static void constructor() {
    test2 = 0x5678;
}

__attribute__((destructor)) static void destructor() {
    test2 = 0x9abc;
}

int main(void) {
    test++;
    test2--;
    return 123;
}
