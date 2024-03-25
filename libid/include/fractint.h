#pragma once

int id_main(int argc, char *argv[]);
bool check_key();

enum class timer_type
{
    ENGINE = 0,
    DECODER = 1,
    ENCODER = 2
};
int timer(timer_type timertype, int(*subrtn)(), ...);
