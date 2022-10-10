#include "bublecs.h"
#include <gtest/gtest.h>

struct Position
{
	int x;
	int y;
};

TEST(EcsTest, BasicQueries)
{
	ecs::World world;

	int e = world.CreateEntity(Position{ 1, 3 });

	int e2 = world.CreateEntity(Position{ 5, 2 }, std::string("name"));

	int i = 0;
	world.ForEach<Position>([&i](Position& p) {
		switch (i) {
			case 0:
				EXPECT_EQ(p.x, 5);
				EXPECT_EQ(p.y, 2);
				break;
			case 1:
				EXPECT_EQ(p.x, 1);
				EXPECT_EQ(p.y, 3);
				break;
			default:
				EXPECT_TRUE(false);
				break;
		}
		i++;
	});

	i = 0;
	world.ForEach<std::string>([&i](std::string& v) {
		EXPECT_EQ(i, 0);
		EXPECT_EQ(v, "name");
		i++;
	});

	i = 0;
	world.ForEach<Position, std::string>([&i](Position& p, std::string& v) {
		EXPECT_EQ(i, 0);
		EXPECT_EQ(p.x, 5);
		EXPECT_EQ(p.y, 2);
		EXPECT_EQ(v, "name");
		i++;
	});

	i = 0;
	world.ForEach<std::string, Position>([&i](std::string& v, Position& p) {
		EXPECT_EQ(i, 0);
		EXPECT_EQ(p.x, 5);
		EXPECT_EQ(p.y, 2);
		EXPECT_EQ(v, "name");
		i++;
	});
}
