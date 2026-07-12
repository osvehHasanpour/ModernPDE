int pick(int flag)
{
    if(flag)
    {
        return 1;
    }

    return 0;
}

int main()
{
    return pick(1) + pick(0);
}
