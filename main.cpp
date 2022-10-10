#include "bublecs.h"
#include "utils.hpp"
#include <exception>
#include <iostream>
#include <vector>

struct Position
{
	int x;
	int y;
};

struct Value
{
	int value;
};

struct Unit
{
	std::string name;
};

int main()
{
	std::set_terminate(catcher);
	ecs::World world;

	ecs::ECSId e = world.CreateEntity(Position{ 5, 2 }, Value{ 123 }, Unit{ "a" });

	world.ForEach<Position, Value, Unit>([](Position& p, Value& v, Unit& u) { std::cout << p.x << " " << p.y << " " << v.value << " " << u.name << std::endl; });

	world.RemoveComponents<Position>(e);

	world.ForEach<Position>([](Position& p) { std::cout << p.x << " " << p.y << std::endl; });

	world.RemoveComponents<Value>(e);

	world.ForEach<Value>([](Value& v) { std::cout << v.value << std::endl; });

	world.RemoveComponents<Unit>(e);

	world.ForEach<Unit>([](Unit& u) { std::cout << u.name << std::endl; });

	return 0;
}
