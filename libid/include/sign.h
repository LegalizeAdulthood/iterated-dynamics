#pragma once

template <typename T>
int sign(T x)
{
    return (T{} < x) - (x < T{});
}
