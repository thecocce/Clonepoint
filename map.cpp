/*
Copyright 2013-2015 Rohit Nirmal

This file is part of Clonepoint.

Clonepoint is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Clonepoint is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clonepoint.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <string>
#include "tinyxml/tinyxml.h"

#include "entity.h"
#include "global.h"
#include "map.h"

#define LIGHTRADIUS 768
#define TILEDIM 16

Map::Map()
{
	_mapTex = 0;
}

Map::~Map()
{
	LOGF((stdout, "Running map destructor!\n"));
	size_t i;
	bool sniper_active = std::find(_entities.begin(), _entities.end(), _sniper) != _entities.end();
	for (i = 0; i < _entities.size(); i++)
	{
		delete _entities[i];
	}

	for (i = 0; i < _tutorials.size(); i++)
	{
		delete _tutorials[i];
	}

	for (i = 0; i < _collideVols.size(); i++)
	{
		delete _collideVols[i];
	}

	for (i = 0; i < _stairDoors.size(); i++)
	{
		delete _stairDoors[i];
	}

	for (i = 0; i < _stairwells.size(); i++)
	{
		delete _stairwells[i];
	}

	for (i = 0; i < _lights.size(); i++)
	{
		delete _lights[i];
	}

	for (i = 0; i < _shafts.size(); i++)
	{
		delete _shafts[i];
	}

	if (!sniper_active)
	{
		delete _sniper;
	}

	_collideVols.clear();
	_entities.clear();
	_linkableEnts.clear();
	_stairDoors.clear();
	_stairwells.clear();
	_lights.clear();
	_shafts.clear();
	_enemyIndices.clear();
	_tutorials.clear();
	glDeleteTextures(1, &_mapTex);
}

bool Map::loadFromFile(const char* filename, bool savegame)
{
	int i = 0;
	char* encoding;
	TiXmlDocument doc(filename);
	_playerStartPos = vec2f(0, 0);
	_subwayPos = vec2f(0, 0);
	_subwayFound = false;
	_backgroundYOffset = 0;

	if (!doc.LoadFile())
	{
		LOGF((stderr, "Failed to load map file %s.\n", filename));
		return false;
	}

	TiXmlHandle hDoc(&doc);
	TiXmlElement *root, *lvl1, *lvl2, *lvl3;
	root = doc.FirstChildElement("map");

	if(!root)
	{
		LOGF((stderr, "Failed to parse map file %s.\n", filename));
		return false;
	}

	_tilesWide = atoi(root->Attribute("width"));
	_tilesHigh = atoi(root->Attribute("height"));

	_mapWidth = _tilesWide * TILEDIM;
	_mapHeight = _tilesHigh * TILEDIM;

	lvl1 = root->FirstChildElement("tileset");
	while(lvl1)
	{
		lvl2 = lvl1->FirstChildElement("image");
		while (lvl2)
		{
			LOGF((stdout, "lvl2 filename = %s\n", lvl2->Attribute("source")));
			if (atoi(lvl1->Attribute("firstgid")) == 1)
			{
				std::string fullSource = std::string("data/") + std::string(lvl2->Attribute("source"));
				_tilesetImage = loadSurfaceFromImage(fullSource.c_str());
			}
			lvl2 = lvl1->NextSiblingElement("image");
		}

		lvl1 = lvl1->NextSiblingElement("tileset");
		i++;
	}

	i = 0;

	lvl1 = root->FirstChildElement("layer");
	while (lvl1)
	{
		lvl2 = lvl1->FirstChildElement("data");

		if (lvl2)
		{
			encoding = (char*)lvl2->Attribute("encoding");

			if (!strcmp(encoding, "csv"))
			{
				parseTileLayer((char*)lvl2->GetText());
			}
		}
		else
		{
			LOGF((stderr, "Error. Layer has no data.\n"));
		}

		lvl1 = lvl1->NextSiblingElement("layer");
		i++;
	}

	i = 0;

	lvl1 = root->FirstChildElement("objectgroup");
	while (lvl1)
	{
		if (!strcmp(lvl1->Attribute("name"), "Collision"))
		{
			lvl2 = lvl1->FirstChildElement("object");
			parseCollisionVolume(lvl2);
		}
		else if (!strcmp(lvl1->Attribute("name"), "RedLinkable"))
		{
			lvl2 = lvl1->FirstChildElement("object");
			parseLinkableObject(lvl2, RED);
		}
		else if (!strcmp(lvl1->Attribute("name"), "GreenLinkable"))
		{
			lvl2 = lvl1->FirstChildElement("object");
			parseLinkableObject(lvl2, GREEN);
		}
		else if (!strcmp(lvl1->Attribute("name"), "BlueLinkable"))
		{
			lvl2 = lvl1->FirstChildElement("object");
			parseLinkableObject(lvl2, BLUE);
		}
		else if (!strcmp(lvl1->Attribute("name"), "VioletLinkable"))
		{
			lvl2 = lvl1->FirstChildElement("object");
			parseLinkableObject(lvl2, VIOLET);
		}
		else if (!strcmp(lvl1->Attribute("name"), "YellowLinkable"))
		{
			lvl2 = lvl1->FirstChildElement("object");
			parseLinkableObject(lvl2, YELLOW);
		}
		else if (!strcmp(lvl1->Attribute("name"), "Links") && !savegame)
		{
			lvl2 = lvl1->FirstChildElement("object");
			while (lvl2)
			{
				lvl3 = lvl2->FirstChildElement("polyline");
				if (lvl3)
				{
					parsePolyline(lvl3, atof(lvl2->Attribute("x")), atof(lvl2->Attribute("y")), &_lines);
				}
				lvl2 = lvl2->NextSiblingElement("object");
			}
		}
		else if (!strcmp(lvl1->Attribute("name"), "LightLinks"))
		{
			lvl2 = lvl1->FirstChildElement("object");
			while (lvl2)
			{
				lvl3 = lvl2->FirstChildElement("polyline");
				if (lvl3)
				{
					parsePolyline(lvl3, atof(lvl2->Attribute("x")), atof(lvl2->Attribute("y")), &_lightLinks);
				}
				lvl2 = lvl2->NextSiblingElement("object");
			}
		}
		else if (!strcmp(lvl1->Attribute("name"), "Entities"))
		{
			lvl2 = lvl1->FirstChildElement("object");
			parseEntity(lvl2, savegame);
		}
		else if (!strcmp(lvl1->Attribute("name"), "Objectives"))
		{
			lvl2 = lvl1->FirstChildElement("object");
			parseObjective(lvl2);
		}
		else if (!strcmp(lvl1->Attribute("name"), "Lights"))
		{
			lvl2 = lvl1->FirstChildElement("object");
			parseLight(lvl2);
		}
		else if (!strcmp(lvl1->Attribute("name"), "Props"))
		{
			lvl2 = lvl1->FirstChildElement("object");
			parseProp(lvl2);
		}
		else if (!strcmp(lvl1->Attribute("name"), "Player"))
		{
			lvl2 = lvl1->FirstChildElement("object");
			if (!strcmp(lvl2->Attribute("type"), "Player"))
			{
				_playerStartPos.x = atoi(lvl2->Attribute("x"));
				_playerStartPos.y = atoi(lvl2->Attribute("y"));
			}
		}

		lvl1 = lvl1->NextSiblingElement("objectgroup");
		i++;
	}

	makeElevatorShafts();
	setShaftOpenings();
	makeStairwells();
	calculateStairDirections();
	calculateElevatorOrder();

	size_t linkableCount = 0;
	for (size_t j = 0; j < _entities.size(); j++)
	{
		if (dynamic_cast<LinkableEntity*>(_entities[j]))
		{
			linkableCount++;
		}
	}
	Assert((_linkableEnts.size() == linkableCount));

	_sniper = new Enemy(0, 0, Left, false, Enemy_Sniper);

	EnemyGun* eg = new EnemyGun(0, 0, RED);
	eg->_enemy = _sniper;
	_sniper->_gun = eg;
	_entities.push_back(eg);

	findBackGroundYOffset(filename);

	return true;
}

size_t Map::getNumberOfCollideVols()
{
	return _collideVols.size();
}

size_t Map::getNumberOfEnts()
{
	return _entities.size();
}

size_t Map::getNumberOfStairs()
{
	return _stairDoors.size();
}

size_t Map::getNumberOfStairwells()
{
	return _stairwells.size();
}

size_t Map::getNumberOfLights()
{
	return _lights.size();
}

size_t Map::getNumberOfLines()
{
	return _lines.size();
}

size_t Map::getNumberOfLightLinks()
{
	return _lightLinks.size();
}

size_t Map::getNumberOfShafts()
{
	return _shafts.size();
}

size_t Map::getNumberOfEnemies()
{
	return _enemyIndices.size();
}

CollisionVolume Map::getCollideVolAt(int i)
{
	return *_collideVols[i];
}

CollisionVolume* Map::getCollideVolPointerAt(int i)
{
	return _collideVols[i];
}

Entity* Map::getEntAt(size_t i)
{
	return _entities[i];
}

Enemy* Map::getEnemyAt(size_t i)
{
	Entity* ent = _entities[_enemyIndices[i]];
	Assert(dynamic_cast<Enemy*>(ent));
	return static_cast<Enemy*>(ent);
}

Stairs* Map::getStairsAt(size_t i)
{
	return _stairDoors[i];
}

Stairwell* Map::getStairwellAt(size_t i)
{
	return _stairwells[i];
}

FieldOfView* Map::getLightAt(size_t i)
{
	return _lights[i];
}

Line Map::getLineAt(size_t i)
{
	return _lines[i];
}

Line Map::getLightLinkAt(size_t i)
{
	return _lightLinks[i];
}

ElevatorShaft* Map::getShaftAt(size_t i)
{
	return _shafts[i];
}

unsigned int Map::getMapWidth()
{
	return _mapWidth;
}

unsigned int Map::getMapHeight()
{
	return _mapHeight;
}

int Map::indexOfCollideVol(CollisionVolume* vol)
{
	unsigned int i;
	for (i = 0; i < _collideVols.size(); i++)
	{
		if (_collideVols[i] == vol)
		{
			return (int)i;
		}
	}
	return -1;
}

int Map::indexOfEntity(Entity* ent)
{
	unsigned int i;
	for (i = 0; i < _entities.size(); i++)
	{
		if (_entities[i] == ent)
		{
			return (int)i;
		}
	}
	return -1;
}

int Map::indexOfFOV(FieldOfView* fov)
{
	unsigned int i;
	for (i = 0; i < _lights.size(); i++)
	{
		if (_lights[i] == fov)
		{
			return (int)i;
		}
	}
	return -1;
}

void Map::parseLinkableObject(TiXmlElement* element, Circuit c)
{
	bool open;
	bool handscanner;
	bool facingRight;
	float r, g, b;
	TiXmlElement* lvl1, *lvl2;
	while (element)
	{
		if (!element->Attribute("x") || !element->Attribute("y") || !element->Attribute("type"))
		{
			LOGF((stderr, "LinkableEntity has null value - ignoring.\n"));
		}
		else
		{
			int x = atoi(element->Attribute("x"));
			int y = atoi(element->Attribute("y"));
			if (!strcmp(element->Attribute("type"), "Switch"))
			{
				handscanner = false;
				lvl1 = element->FirstChildElement("properties");

				if (lvl1)
				{
					lvl2 = lvl1->FirstChildElement("property");
					while (lvl2)
					{
						if (lvl2->Attribute("name") && !strcmp(lvl2->Attribute("name"), "handscanner") &&
						        lvl2->Attribute("value") && !strcmp(lvl2->Attribute("value"), "yes"))
						{
							handscanner = true;
						}

						lvl2 = lvl2->NextSiblingElement("property");
					}
				}

				LightSwitch* sw = new LightSwitch(x, y - ENTDIM, c, handscanner);
				_entities.push_back(sw);
				_linkableEnts.push_back(sw);
			}
			else if (!strcmp(element->Attribute("type"), "Door"))
			{
				open = false;

				lvl1 = element->FirstChildElement("properties");

				if (lvl1)
				{
					lvl2 = lvl1->FirstChildElement("property");
					while (lvl2)
					{
						if (lvl2->Attribute("name") && !strcmp(lvl2->Attribute("name"), "open") &&
						        lvl2->Attribute("value") && !strcmp(lvl2->Attribute("value"), "yes"))
						{
							open = true;
						}

						lvl2 = lvl2->NextSiblingElement("property");
					}
				}

				Door* door = new Door(x, y - ENTDIM, c, open ? true : false, Door_Normal);
				_collideVols.push_back(door->getCollisionVolume());
				_entities.push_back(door);
				_linkableEnts.push_back(door);
			}
			else if (!strcmp(element->Attribute("type"), "TrapDoor"))
			{
				Door* td = new Door(x, y - ENTDIM, c, false, Door_Trap);
				_collideVols.push_back(td->getCollisionVolume());
				_entities.push_back(td);
				_linkableEnts.push_back(td);
			}
			else if (!strcmp(element->Attribute("type"), "VaultDoor"))
			{
				Door* vd = new Door(x, y - ENTDIM, c, false, Door_Vault);
				_collideVols.push_back(vd->getCollisionVolume());
				_collideVols.push_back(vd->getCollisionVolume2());
				_entities.push_back(vd);
				_linkableEnts.push_back(vd);
			}
			else if (!strcmp(element->Attribute("type"), "LightFixture"))
			{
				r = g = b = 1.0f;
				lvl1 = element->FirstChildElement("properties");
				if (lvl1)
				{
					lvl2 = lvl1->FirstChildElement("property");
					while (lvl2)
					{
						if (lvl2->Attribute("name") && !strcmp(lvl2->Attribute("name"), "red") &&
						        lvl2->Attribute("value"))
						{
							r = atof(lvl2->Attribute("value"));
						}

						if (lvl2->Attribute("name") && !strcmp(lvl2->Attribute("name"), "green") &&
						        lvl2->Attribute("value"))
						{
							g = atof(lvl2->Attribute("value"));
						}

						if (lvl2->Attribute("name") && !strcmp(lvl2->Attribute("name"), "blue") &&
						        lvl2->Attribute("value"))
						{
							b = atof(lvl2->Attribute("value"));
						}

						lvl2 = lvl2->NextSiblingElement("property");
					}
				}

				LightFixture* fixture = new LightFixture(x, y - ENTDIM, c, true);
				FieldOfView* fov = new FieldOfView(x + (ENTDIM / 2) - 8, y - (ENTDIM / 2) + 4, LIGHTRADIUS, 0, 180, true, FOV_LIGHT);
				fov->_red = r;
				fov->_green = g;
				fov->_blue = b;
				fixture->addFOV(fov);
				_entities.push_back(fixture);
				_linkableEnts.push_back(fixture);
				_lights.push_back(fov);
			}
			else if (!strcmp(element->Attribute("type"), "Socket"))
			{
				PowerSocket* socket = new PowerSocket(x, y - ENTDIM, c);
				_entities.push_back(socket);
				_linkableEnts.push_back(socket);
			}
			else if (!strcmp(element->Attribute("type"), "Scanner"))
			{
				MotionScanner* scanner = new MotionScanner(x, y - ENTDIM, c);
				_entities.push_back(scanner);
				_linkableEnts.push_back(scanner);
			}
			else if (!strcmp(element->Attribute("type"), "Elevator"))
			{
				ElevatorDoor* ed = new ElevatorDoor(x, y - ENTDIM);
				ElevatorSwitch* sw = new ElevatorSwitch(x + 32, y - ENTDIM, c);
				sw->_door = ed;
				ed->_switch = sw;
				_entities.push_back(ed);
				_entities.push_back(sw);
				_linkableEnts.push_back(sw);
			}
			else if (!strcmp(element->Attribute("type"), "SoundDetector"))
			{
				SoundDetector* sd = new SoundDetector(x, y - ENTDIM, c);
				_entities.push_back(sd);
				_linkableEnts.push_back(sd);
			}
			else if (!strcmp(element->Attribute("type"), "SecurityCamera"))
			{
				facingRight = false;

				lvl1 = element->FirstChildElement("properties");

				if (lvl1)
				{
					lvl2 = lvl1->FirstChildElement("property");
					while (lvl2)
					{
						if (lvl2->Attribute("name") && !strcmp(lvl2->Attribute("name"), "direction") &&
						        lvl2->Attribute("value") && !strcmp(lvl2->Attribute("value"), "right"))
						{
							facingRight = true;
						}

						lvl2 = lvl2->NextSiblingElement("property");
					}
				}

				FieldOfView* fov = new FieldOfView(facingRight ? x + ENTDIM/2 + 8 : x + ENTDIM/2 - 8,
				                                 y - ENTDIM/2 + 8, LIGHTRADIUS, facingRight ? 27 : -27, 27, true, FOV_CAMERA);

				fov->_red = 0.5f;
				fov->_green = 0.5f;
				fov->_blue = 0.0f;

				SecurityCamera* cam = new SecurityCamera(x , y - ENTDIM, c, facingRight ? Right : Left, fov);
				_entities.push_back(cam);
				_linkableEnts.push_back(cam);
				_lights.push_back(fov);
			}
			else if (!strcmp(element->Attribute("type"), "Alarm"))
			{
				Alarm* alarm = new Alarm(x, y - ENTDIM, c);
				_entities.push_back(alarm);
				_linkableEnts.push_back(alarm);
			}
		}
		element = element->NextSiblingElement("object");
	}
}

void Map::parseCollisionVolume(TiXmlElement* element)
{
	while (element)
	{
		if (!element->Attribute("x") || !element->Attribute("y") || !element->Attribute("width") || !element->Attribute("height"))
		{
			LOGF((stderr, "Collision volume has null value - ignoring.\n"));
		}
		else
		{
			CollisionVolume* vol = new CollisionVolume();
			vol->flags = 0;
			vol->rect.x = atoi(element->Attribute("x"));
			vol->rect.y = atoi(element->Attribute("y"));
			vol->rect.w = atoi(element->Attribute("width"));
			vol->rect.h = atoi(element->Attribute("height"));
			if (element->Attribute("type") && !strcmp(element->Attribute("type"), "Glass"))
			{
				vol->flags |= COLLISION_GLASS;
			}
			if (element->Attribute("type") && !strcmp(element->Attribute("type"), "GuardBlock"))
			{
				vol->flags |= COLLISION_GUARDBLOCK;
			}
			if (!vol->guardblock())
			{
				vol->flags |= COLLISION_ACTIVE;
			}

			_collideVols.push_back(vol);
		}
		element = element->NextSiblingElement("object");
	}
}

void Map::parseEntity(TiXmlElement* element, bool savegame)
{
	bool facingRight;
	bool startPatrolling;
	TiXmlElement* lvl1, *lvl2;
	while (element)
	{
		facingRight = false;
		startPatrolling = false;

		if (!element->Attribute("x") || !element->Attribute("y") || !element->Attribute("type"))
		{
			LOGF((stderr, "Entity has null value - ignoring.\n"));
		}
		else
		{
			int x = atoi(element->Attribute("x"));
			int y = atoi(element->Attribute("y"));

			lvl1 = element->FirstChildElement("properties");

			if (lvl1)
			{
				lvl2 = lvl1->FirstChildElement("property");
				while (lvl2)
				{
					if (lvl2->Attribute("name") && !strcmp(lvl2->Attribute("name"), "direction") &&
					        lvl2->Attribute("value") && !strcmp(lvl2->Attribute("value"), "right"))
					{
						facingRight = true;
					}

					if (lvl2->Attribute("name") && !strcmp(lvl2->Attribute("name"), "patrolling") &&
					        lvl2->Attribute("value") && !strcmp(lvl2->Attribute("value"), "yes"))
					{
						startPatrolling = true;
					}

					lvl2 = lvl2->NextSiblingElement("property");
				}
			}

			if (!strcmp(element->Attribute("type"), "Guard"))
			{
				Enemy* guard = new Enemy(x, y - ENTDIM, facingRight ? Right : Left, startPatrolling, Enemy_Guard);
				_entities.push_back(guard);
				_enemyIndices.push_back(_entities.size() - 1);
				EnemyGun* eg = new EnemyGun(x, y, RED);
				eg->_enemy = guard;
				guard->_gun = eg;
				_entities.push_back(eg);
				_linkableEnts.push_back(eg);
			}

			if (!strcmp(element->Attribute("type"), "Enforcer"))
			{
				Enemy* enforcer = new Enemy(x, y - ENTDIM, facingRight ? Right : Left, startPatrolling, Enemy_Enforcer);
				_entities.push_back(enforcer);
				_enemyIndices.push_back(_entities.size() - 1);
				EnemyGun* eg = new EnemyGun(x, y, RED);
				eg->_enemy = enforcer;
				enforcer->_gun = eg;
				_entities.push_back(eg);
				_linkableEnts.push_back(eg);
			}

			if (!strcmp(element->Attribute("type"), "Professional"))
			{
				Enemy* professional = new Enemy(x, y - ENTDIM, facingRight ? Right : Left, startPatrolling, Enemy_Professional);
				_entities.push_back(professional);
				_enemyIndices.push_back(_entities.size() - 1);
				EnemyGun* eg = new EnemyGun(x, y, RED);
				eg->_enemy = professional;
				professional->_gun  = eg;
				_entities.push_back(eg);
				_linkableEnts.push_back(eg);
			}

			if (!strcmp(element->Attribute("type"), "Stairs"))
			{
				_stairDoors.push_back(new Stairs(x, y - ENTDIM));
			}

			if (!strcmp(element->Attribute("type"), "CircuitBoxG"))
			{
				_entities.push_back(new CircuitBox(x, y - ENTDIM, GREEN));
			}

			if (!strcmp(element->Attribute("type"), "CircuitBoxB"))
			{
				_entities.push_back(new CircuitBox(x, y - ENTDIM, BLUE));
			}

			if (!strcmp(element->Attribute("type"), "CircuitBoxY"))
			{
				_entities.push_back(new CircuitBox(x, y - ENTDIM, YELLOW));
			}

			if (!strcmp(element->Attribute("type"), "CircuitBoxV"))
			{
				_entities.push_back(new CircuitBox(x, y - ENTDIM, VIOLET));
			}

			if (!strncmp(element->Attribute("type"), "tut_", 4))
			{
				StringMessage sm = NUMBER_OF_STRING_MESSAGES;
				if (!strcmp(element->Attribute("type"), "tut_01"))
				{
					sm = SM_Start;
				}
				if (!strcmp(element->Attribute("type"), "tut_02"))
				{
					sm = SM_Jumping;
				}
				if (!strcmp(element->Attribute("type"), "tut_03"))
				{
					sm = SM_Falling;
				}
				if (!strcmp(element->Attribute("type"), "tut_04"))
				{
					sm = SM_Guards1;
				}
				if (!strcmp(element->Attribute("type"), "tut_05"))
				{
					sm = SM_Guards2;
				}
				if (!strcmp(element->Attribute("type"), "tut_06"))
				{
					sm = SM_Crosslink1;
				}
				if (!strcmp(element->Attribute("type"), "tut_07"))
				{
					sm = SM_Crosslink2;
				}
				if (!strcmp(element->Attribute("type"), "tut_08"))
				{
					sm = SM_Objectives;
				}
				if (!strcmp(element->Attribute("type"), "tut_09"))
				{
					sm = SM_Elevators;
				}
				if (!strcmp(element->Attribute("type"), "tut_10"))
				{
					sm = SM_Optional;
				}
				if (!strcmp(element->Attribute("type"), "tut_11"))
				{
					sm = SM_Circuits;
				}
				if (sm < NUMBER_OF_STRING_MESSAGES)
				{
					_tutorials.push_back(new TutorialMark(x, y - ENTDIM, sm));
				}
			}
		}
		element = element->NextSiblingElement("object");
	}
}

void Map::parseLight(TiXmlElement* element)
{
	int direction, numangles;
	TiXmlElement* lvl1, *lvl2;

	while (element)
	{
		if (!element->Attribute("x") || !element->Attribute("y"))
		{
			LOGF((stderr, "Light has null value - ignoring.\n"));
		}
		else
		{
			direction = 0;
			numangles = 180;
			int x = atoi(element->Attribute("x"));
			int y = atoi(element->Attribute("y"));

			lvl1 = element->FirstChildElement("properties");

			if (lvl1)
			{
				lvl2 = lvl1->FirstChildElement("property");
				while (lvl2)
				{
					if (lvl2->Attribute("name") && !strcmp(lvl2->Attribute("name"), "numangles") &&
					        lvl2->Attribute("value"))
					{
						numangles = atoi(lvl2->Attribute("value"));
					}

					if (lvl2->Attribute("name") && !strcmp(lvl2->Attribute("name"), "direction") &&
					        lvl2->Attribute("value"))
					{
						direction = atoi(lvl2->Attribute("value"));
					}

					lvl2 = lvl2->NextSiblingElement("property");
				}
			}

			FieldOfView* fov = new FieldOfView(x, y + 4, LIGHTRADIUS, direction, numangles, true, FOV_LIGHT);
			_lights.push_back(fov);

		}
		element = element->NextSiblingElement("object");
	}
}

void Map::parseProp(TiXmlElement* element)
{
	std::string prop = "";
	while (element)
	{
		if (!element->Attribute("x") || !element->Attribute("y"))
		{
			LOGF((stderr, "Prop has null value - ignoring.\n"));
		}
		else
		{
			int x = atoi(element->Attribute("x"));
			int y = atoi(element->Attribute("y"));

			if (!strcmp(element->Attribute("type"), "Watercooler"))
			{
				prop = "watercooler";
			}

			if (!strcmp(element->Attribute("type"), "Shelf"))
			{
				prop = "shelf";
			}

			if (!strcmp(element->Attribute("type"), "Trashcan"))
			{
				prop = "trashcan";
			}

			if (!strcmp(element->Attribute("type"), "Couch"))
			{
				prop = "couch";
			}

			if (!strcmp(element->Attribute("type"), "Plant1"))
			{
				prop = "plant1";
			}

			if (!strcmp(element->Attribute("type"), "Plant2"))
			{
				prop = "plant2";
			}

			if (!strcmp(element->Attribute("type"), "SubwaySign"))
			{
				prop = "subwaysign";
			}

			if (!strcmp(element->Attribute("type"), "LightPanel"))
			{
				prop = "lightpanel";
			}

			if (prop != "")
			{
				_entities.push_back(new Entity(x, y, Locator::getSpriteManager()->getIndex("./data/sprites/objects.sprites", prop)));
				_entities[_entities.size() - 1]->setCollisionRectDimsAndPosition(x, y - ENTDIM, ENTDIM, ENTDIM, ENTDIM);
			}
		}
		element = element->NextSiblingElement("object");
		prop = "";
	}
}

void Map::parsePolyline(TiXmlElement* element, float ox, float oy, std::vector<Line>* container)
{
	std::string points = std::string(element->Attribute("points"));
	std::string p1 = points.substr(0, points.find(" "));
	std::string p2 = points.substr(points.find(" ") + 1);
	float x, y;

	Line line;
	getPolylineComponent(p1, &x, &y);
	line.p1 = vec2f(x + ox, y + oy);
	getPolylineComponent(p2, &x, &y);
	line.p2 = vec2f(x + ox, y + oy);

	container->push_back(line);
}

void Map::getPolylineComponent(std::string point, float* x, float* y)
{
	std::string p1 = point.substr(0, point.find(","));
	std::string p2 = point.substr(point.find(",") + 1);

	*x = atof(p1.c_str());
	*y = atof(p2.c_str());
}

void Map::parseObjective(TiXmlElement* element)
{
	while (element)
	{
		if (!element->Attribute("x") || !element->Attribute("y") || !element->Attribute("type"))
		{
			LOGF((stderr, "Objective has null value - ignoring.\n"));
		}
		else
		{
			int x = atoi(element->Attribute("x"));
			int y = atoi(element->Attribute("y"));

			if (!strcmp(element->Attribute("type"), "Terminal"))
			{
				_entities.push_back(new MainComputer(x, y - ENTDIM, true));
			}
			else if (!strcmp(element->Attribute("type"), "Subway"))
			{
				_subwayPos.x = x;
				_subwayPos.y = y;
				_subwayFound = true;
			}
		}
		element = element->NextSiblingElement("object");
	}
}

void Map::makeElevatorShafts()
{
	size_t i;
	ElevatorDoor* ed;
	int iShaft;
	for (i = 0; i < _entities.size(); i++)
	{
		if (dynamic_cast<ElevatorDoor*>(_entities[i]))
		{
			ed = (ElevatorDoor*)_entities[i];
			iShaft = doesElevatorShaftExist((int)ed->getPosition().x);
			if (iShaft >= 0) //shaft already exists, just add to that.
			{
				_shafts[iShaft]->addDoor(ed);
				ed->_shaft = _shafts[iShaft];
			}
			else
			{
				ElevatorShaft* shaft = new ElevatorShaft((int)ed->getPosition().x);
				shaft->addDoor(ed);
				ed->_shaft = shaft;
				_shafts.push_back(shaft);
			}
		}
	}
#if _WIN32
	LOGF((stdout, "Created %i elevator shaft(s).\n", _shafts.size()));
#else
	LOGF((stdout, "Created %lu elevator shaft(s).\n", _shafts.size()));
#endif
}

int Map::doesElevatorShaftExist(int x)
{
	size_t i;
	for (i = 0; i < _shafts.size(); i++)
	{
		if (_shafts[i]->_x == x)
		{
			return (int)i;
		}
	}
	return -1;
}

void Map::makeStairwells()
{
	size_t i;
	Stairs* stairs;
	int iStairWell;
	for (i = 0; i < _stairDoors.size(); i++)
	{
		stairs = _stairDoors[i];
		iStairWell = doesStairwellExist((int)stairs->getPosition().x);
		if (iStairWell >= 0) //stairwell already exists, just add to that.
		{
			_stairwells[iStairWell]->addStairs(stairs);
			stairs->registerStairwell(_stairwells[iStairWell]);
		}
		else
		{
			Stairwell* well = new Stairwell((int)stairs->getPosition().x);
			well->addStairs(stairs);
			stairs->registerStairwell(well);
			_stairwells.push_back(well);
		}
	}
#if _WIN32
	LOGF((stdout, "Created %i stairwell(s).\n", _shafts.size()));
#else
	LOGF((stdout, "Created %lu stairwell(s).\n", _shafts.size()));
#endif
}

int Map::doesStairwellExist(int x)
{
	size_t i;
	for (i = 0; i < _stairwells.size(); i++)
	{
		if (_stairwells[i]->getX() == x)
		{
			return (int)i;
		}
	}
	return -1;
}

void Map::calculateStairDirections()
{
	size_t i;

	for (i = 0; i < _stairwells.size(); i++)
	{
		_stairwells[i]->setDirections(_mapHeight);
	}
}

void Map::calculateElevatorOrder()
{
	size_t i;

	for (i = 0; i < _shafts.size(); i++)
	{
		_shafts[i]->calculateDoorOrders(_mapHeight);
	}
}

void Map::setShaftOpenings()
{
	size_t i;
	for (i = 0; i < _shafts.size(); i++)
	{
		_shafts[i]->setOpenDoorFirst();
	}
}

vec2f Map::getPlayerStartPos()
{
	return _playerStartPos;
}

void Map::addSavedLink(vec2f start, vec2f end)
{
	Line line = {start, end};
	_lines.push_back(line);
}

void Map::parseTileLayer(char* data)
{
	if (!_tilesetImage)
	{
		_mapTex = 0;
		return;
	}

	SDL_Surface* mapImage;
	SDL_Surface* mapImageCrosslink;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	mapImage = SDL_CreateRGBSurface(SDL_SWSURFACE, _mapWidth, _mapHeight, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
	mapImageCrosslink = SDL_CreateRGBSurface(SDL_SWSURFACE, _mapWidth, _mapHeight, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#else
	mapImage = SDL_CreateRGBSurface(SDL_SWSURFACE, _mapWidth, _mapHeight, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	mapImageCrosslink = SDL_CreateRGBSurface(SDL_SWSURFACE, _mapWidth, _mapHeight, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#endif

	SDL_FillRect(mapImage, NULL, 0xFF00FF);
	SDL_FillRect(mapImageCrosslink, NULL, 0xFF00FF);

	char* delim = (char*)",";
	char* token = strtok(data, delim);
	unsigned int i = 0;
	int gid;
	int tileset_col;
	int tileset_row;
	SDL_Rect srcrect;
	SDL_Rect dstrect;
	unsigned int row;
	unsigned int col;
	int endcol;
	unsigned int tilesetWidthInTiles = _tilesetImage->w / TILEDIM;

	while (token)
	{
		i++;
		gid = atoi(token);

		if (gid > 0)
		{
			gid--;

			row = i / _tilesWide;
			endcol = i - (row * _tilesWide) - 1;
			if (endcol < 0)
			{
				row--;
			}
			col = i - (row * _tilesWide) - 1;

			tileset_col = gid % tilesetWidthInTiles;
			tileset_row = gid / tilesetWidthInTiles;

			srcrect.x = TILEDIM * tileset_col;
			srcrect.y = TILEDIM * tileset_row;
			srcrect.w = TILEDIM;
			srcrect.h = TILEDIM;

			dstrect.x = col * TILEDIM;
			dstrect.y = row * TILEDIM;
			dstrect.w = TILEDIM;
			dstrect.h = TILEDIM;

			SDL_BlitSurface(_tilesetImage, &srcrect, mapImage, &dstrect);
			SDL_FillRect(mapImageCrosslink, &dstrect, 0x500000);
		}

		if (token)
			token = strtok(NULL, delim);
	}

	_mapTex = createTextureFromSurface(mapImage);
	_crosslinkTex = createTextureFromSurface(mapImageCrosslink);

	SDL_FreeSurface(mapImage);
	SDL_FreeSurface(mapImageCrosslink);
	SDL_FreeSurface(_tilesetImage);

	delete [] token;
}

void Map::findBackGroundYOffset(std::string filename)
{
	TiXmlDocument doc(filename.c_str());

	if (!doc.LoadFile())
	{
		LOGF((stderr, "Failed to load map file %s.\n", filename.c_str()));
		return;
	}

	TiXmlHandle hDoc(&doc);
	TiXmlElement *root, *lvl1, *lvl2;
	root = doc.FirstChildElement("map");

	if(!root)
	{
		LOGF((stderr, "Failed to parse map file %s.\n", filename.c_str()));
		return;
	}

	lvl1 = root->FirstChildElement("properties");
	while(lvl1)
	{
		lvl2 = lvl1->FirstChildElement("property");
		while (lvl2)
		{
			if (!strcmp(lvl2->Attribute("name"), "backgroundyoff"))
			{
				_backgroundYOffset = atof(lvl2->Attribute("value")) * TILEDIM;
			}

			lvl2 = lvl2->NextSiblingElement("property");
		}
		lvl1 = lvl1->NextSiblingElement("properties");
	}
}

GLuint Map::getMapTexture()
{
	return _mapTex;
}

GLuint Map::getCrosslinkTexture()
{
	return _crosslinkTex;
}

void Map::clearLinks()
{
	_lines.clear();
	size_t i;
	for (i = 0; i < _linkableEnts.size(); i++)
	{
		_linkableEnts[i]->unlink();
	}
}

bool Map::subwayFound()
{
	return _subwayFound;
}

vec2f Map::getSubwayPosition()
{
	return _subwayPos;
}

void Map::getLinkableIters(std::vector<LinkableEntity*>::iterator* begin,
	                      std::vector<LinkableEntity*>::iterator* end)
{
	*begin = _linkableEnts.begin();
	*end = _linkableEnts.end();
}

void Map::getTutorialIters(std::vector<TutorialMark*>::iterator* begin,
	                      std::vector<TutorialMark*>::iterator* end)
{
	*begin = _tutorials.begin();
	*end = _tutorials.end();
}

void Map::removeEnemyGun(EnemyGun* gun)
{
	size_t i;
	int gunIndex = -1;
	for (i = 0; i < _linkableEnts.size(); i++)
	{
		if (_linkableEnts[i] == gun)
		{
			gunIndex = i;
			break;
		}
	}

	if (gunIndex < 0)
	{
		return;
	}
	_linkableEnts.erase(_linkableEnts.begin() + gunIndex);
}

void Map::addMissingGuns()
{
	size_t i;
	EnemyGun* eg;
	for (i = 0; i < _entities.size(); i++)
	{
		if (dynamic_cast<EnemyGun*>(_entities[i]))
		{
			eg = static_cast<EnemyGun*>(_entities[i]);
			if (eg->_enemy->getType() == Enemy_Sniper)
			{
				continue;
			}
			if (std::find(_linkableEnts.begin(), _linkableEnts.end(), eg) == _linkableEnts.end())
			{
				_linkableEnts.push_back(eg);
			}
		}
	}
}

void Map::addSniper()
{
	if (_subwayFound && std::find(_entities.begin(), _entities.end(), _sniper) == _entities.end())
	{
		_sniper->setAlive(true);
		_sniper->loseSightOfPlayer(false, vec2f(0, 0));
		_entities.push_back(_sniper);
		_enemyIndices.push_back(_entities.size() - 1);
		_sniper->setCollisionRectPosition(_subwayPos.x + ENTDIM - _sniper->getCollisionRect().w, _subwayPos.y - _sniper->getCollisionRect().h);
		_sniper->alertToPosition(_subwayPos.x - 40, _subwayPos.y, ALERT_RUN, TARGET_PLAYER);
	}
}

void Map::removeSniper()
{
	size_t i;
	int sniperIndex = -1;
	int indexInIndices = -1; //index in enemyIndices
	for (i = 0; i < _entities.size(); i++)
	{
		if (_entities[i] == _sniper)
		{
			sniperIndex = i;
			break;
		}
	}

	if (sniperIndex == -1)
	{
		return;
	}

	for (i = 0; i < _enemyIndices.size(); i++)
	{
		if (_enemyIndices[i] == (size_t)sniperIndex)
		{
			indexInIndices = i;
			break;
		}
	}

	Assert((indexInIndices >= 0));

	_entities.erase(_entities.begin() + sniperIndex);
	_enemyIndices.erase(_enemyIndices.begin() + indexInIndices);
	_sniper->changeState(IDLE);
}

Enemy* Map::getSniper()
{
	return _sniper;
}

float Map::getBackGroundYOffset()
{
	return _backgroundYOffset;
}