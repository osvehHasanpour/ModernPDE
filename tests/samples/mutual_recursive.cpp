void odd(int);
void even(int);

void odd(int n)
{
    if(n==0) return;

    even(n-1);
}

void even(int n)
{
    if(n==0) return;

    odd(n-1);
}

int main()
{
    even(5);
}