#include <cstdlib>

int main()
{
    int x=10;

    if(rand()%2)
    {
        x++;
    }
    else
    {
        x--;
    }

    return x;
}