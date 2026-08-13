#include "Worklist.h"

void Worklist::push(int item)
{
    if(membership_.insert(item).second)
    {
        items_.push(item);
    }
}

bool Worklist::empty() const
{
    return items_.empty();
}

int Worklist::pop()
{
    if(items_.empty())
    {
        return -1;
    }

    const int item = items_.front();
    items_.pop();
    membership_.erase(item);
    return item;
}

std::size_t Worklist::size() const
{
    return items_.size();
}

void Worklist::clear()
{
    while(!items_.empty())
    {
        items_.pop();
    }

    membership_.clear();
}
