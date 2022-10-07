#include "ecs.h"

#include <iostream>

struct Position
{
	int x;
	int y;
};

struct Value
{
	int value;
};

int main()
{
    World world;

    Value val;

    int e = world.CreateEntity(Position{1, 3});

    int e2 = world.CreateEntity(Position{5, 2}, Value{123});

    world.for_each<Position>([](Position &p)
                             { std::cout << p.x << " " << p.y << std::endl; });

    world.for_each<Value>([](Value &v)
                          { std::cout << v.value << std::endl; });

    world.for_each<Position, Value>([](Position &p, Value &v)
                                    { std::cout << p.x << " " << v.value << std::endl; });

    world.for_each<Value, Position>([](Value &v, Position &p)
                                    { std::cout << p.x << " " << v.value << std::endl; });
}