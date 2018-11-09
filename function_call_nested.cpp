
#include <cmath>
#include <cstdlib>

int global = 100;

int h(int x)
{
    return 100 * x * std::log((double)x);
}

int f(int x, int y)
{
    return 10*x + h(y/2);
}

int g(int x)
{
    return h(100 + x + std::pow((double)global, 3.0));
}

int i(int x1, int x2, int x3)
{
    return f(100, x1) * x1 * x2 + x3 * f(2, 5);
}

int main(int argc, char ** argv)
{
    int x1 = atoi(argv[1]);
    int x2 = atoi(argv[2]);
    int y = 2*x1 + 1;

    // pass param, pass global
    f(global, x1);
    f(x2, 100);
    // pass nothing, access global
    g(100);
    // pass nothing, access nothing
    h(x2);
    h(100);
    // pass two args, pass arg, nothing
    i(y + x2, x1, 15*global);

    return 0;
}
