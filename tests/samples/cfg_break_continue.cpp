int main()
{
    int x = 0;

    while(x < 10)
    {
        x++;

        if(x == 5)
        {
            break;
        }

        if(x % 2 == 0)
        {
            continue;
        }
    }

    return x;
}
