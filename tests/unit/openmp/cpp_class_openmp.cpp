#include <cstdint>
#include <cstdlib>

#include <omp.h>

#include "perf-taint/PerfTaint.hpp"

class Grid
{
    int x, y, z;
    double * data;
public:
    Grid(int _x, int _y, int _z):
        x(_x),
        y(_y),
        z(_z),
        data(new double[x*y])
        {}
    ~Grid()
    {
        delete[] data;
    }

    void update_grid(double shift)
    {
        #pragma omp parallel
        for(int i = 0; i < x*y;++i)
            data[i] += shift;
    }    
    
    void update_constant(double shift)
    {
        data[0] += 2*shift;
        #pragma omp parallel
        for(int i = 1; i < 5;++i)
            data[i] += shift;
    }    
    
    // not important - data is not influenced by label
    void update_data(double shift)
    {
        data[0] += 2*shift;
        #pragma omp parallel
        for(int i = data[0]; i < 5;++i)
            data[i] += shift;
    } 
    
    // important
    void update_y(double shift)
    {
        #pragma omp parallel
        for(int i = 0; i < 100; i += y)
            data[i] += shift;
    } 
    
    // not important - third field z is not labelled
    void update_z(double shift)
    {
        #pragma omp parallel
        for(int i = 0; i < z;++i)
            data[i] += shift;
    }    
};

int main(int argc, char ** argv)
{
    int size_x EXTRAP = atoi(argv[1]);
    int size_y EXTRAP = atoi(argv[2]);
    int size_z EXTRAP = atoi(argv[3]);

    Grid * g;
    register_variable(&size_x, VARIABLE_NAME(size_x));
    register_variable(&size_y, VARIABLE_NAME(size_y));
    g = new Grid(size_x, size_y, size_z);

    g->update_grid(2.0);
    g->update_constant(1.5);
    g->update_y(1.5);
    g->update_data(1.5);
    g->update_z(1.5);
    
    delete g;
    return 0;
}
