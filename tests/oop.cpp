class Character
{
public:
    virtual void attack(){}
};

class Warrior : public Character
{
public:
    void attack() override{}
};

class Mage : public Character
{
public:
    void attack() override{}
};

int main()
{
    Character* c;

    if(rand()%2)
        c=new Warrior();
    else
        c=new Mage();

    c->attack();
}