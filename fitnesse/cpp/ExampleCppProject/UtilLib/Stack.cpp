#include "Stack.h"

Stack::Stack()
:index(0)
{
}

Stack::~Stack()
{
}

bool Stack::IsEmpty()
{
    return index == 0;
}

void Stack::Push(int value)
{
    values[index++] = value;
}

int Stack::Pop()
{
    return values[--index];
}
