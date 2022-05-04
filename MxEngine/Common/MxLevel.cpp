#include "MxLevel.h"

UINT MxLevel::GlobalIDManager = 0;

MxLevel::MxLevel()
{

}

UINT MxLevel::AddObject(MxObject& object)
{
	UINT Id = GlobalIDManager++;
	object.ObjectID = Id;
	LevelObjectsMap[Id] = std::make_shared<MxObject>(object);
	return Id;
}

