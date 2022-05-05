#include "MxObject.h"
#include "MxRenderComponent.h"

MxObject::MxObject(const std::string& name)
{
	ObjectName = name;
	RenderComponent = std::make_shared<MxRenderComponent>();
}

MxObject::MxObject()
{

}
