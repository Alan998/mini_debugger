void a() {
	long foo = 1;
}

void b() {
	long foo = 2;
	a();
}
void c() {
	long foo = 3;
	b();
}

void d() {
	long foo = 4;
	c();
}

void e() {
	long foo = 5;
	d();
}

void f() {
	long foo = 6;
	e();
}

int main(void) {
	f();
	return 0;
}
