/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2016 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "cellapp.h"
#include "entity.h"
#include "rotator_handler.h"
#include "turn_controller.h"

namespace KBEngine{	


//-------------------------------------------------------------------------------------
RotatorHandler::RotatorHandler(KBEShared_ptr<Controller> pController, const Direction3D& destDir, float velocity, PyObject* userarg):
destDir_(destDir),
velocity_(velocity),
pyuserarg_(userarg),
pController_(pController)
{
	Py_INCREF(userarg);

	static_cast<TurnController*>(pController.get())->pRotatorHandler(this);
	Cellapp::getSingleton().addUpdatable(this);
}

//-------------------------------------------------------------------------------------
RotatorHandler::RotatorHandler() :
destDir_(0.f,0.f,0.f),
velocity_(0.f),
pyuserarg_(NULL),
pController_(KBEShared_ptr<Controller>())
{
}

//-------------------------------------------------------------------------------------
RotatorHandler::~RotatorHandler()
{
	if (pyuserarg_ != NULL)
	{
		Py_DECREF(pyuserarg_);
	}
}

//-------------------------------------------------------------------------------------
void RotatorHandler::addToStream(KBEngine::MemoryStream& s)
{
	s << destDir_.dir.x << destDir_.dir.y << destDir_.dir.z << velocity_;
	s.appendBlob(script::Pickler::pickle(pyuserarg_));
}

//-------------------------------------------------------------------------------------
void RotatorHandler::createFromStream(KBEngine::MemoryStream& s)
{
	s >> destDir_.dir.x >> destDir_.dir.y >> destDir_.dir.z >> velocity_;
	
	std::string val = "";
	s.readBlob(val);
	pyuserarg_ = script::Pickler::unpickle(val);
}

//-------------------------------------------------------------------------------------
bool RotatorHandler::requestTurnOver()
{
	if (pController_)
	{
		if (pController_->pEntity())
			pController_->pEntity()->onTurn(pController_->id(), pyuserarg_);

		// 如果在onTurn中调用cancelController（id）会导致Controller析构导致pController_为NULL
		if (pController_)
			pController_->destroy();
	}

	return true;
}

//-------------------------------------------------------------------------------------
const Direction3D& RotatorHandler::destDir()
{
	return destDir_;
}

//-------------------------------------------------------------------------------------
bool RotatorHandler::update()
{
	if(pController_ == NULL)
	{
		delete this;
		return false;
	}
		
	Entity* pEntity = pController_->pEntity();
	const Direction3D& dstDir = destDir();
	Direction3D currDir = pEntity->direction();

	// 得到差值
	float deltaYaw = dstDir.yaw() - currDir.yaw();

	if (deltaYaw > KBE_PI)
		deltaYaw = (float)((double)deltaYaw - KBE_2PI/* 由于我们的弧度表示范围在-PI ~ PI，此处防止溢出 */);
	else if (deltaYaw < -KBE_PI)
		deltaYaw = (float)((double)deltaYaw + KBE_2PI);

	if (fabs(deltaYaw) < 0.01f)
		deltaYaw = 0.f;
	else if (fabs(deltaYaw) > velocity_)
		deltaYaw = KBEClamp(-velocity_, deltaYaw, velocity_);

	currDir.yaw(currDir.yaw() + deltaYaw);
	
	if (currDir.yaw() > KBE_PI)
		currDir.yaw((float((double)currDir.yaw() - KBE_2PI)));
	else if (currDir.yaw() < -KBE_PI)
		currDir.yaw((float((double)currDir.yaw() + KBE_2PI)));

	// 设置entity的新位置和面向
	if (pController_)
		pEntity->setPositionAndDirection(pEntity->position(), currDir);

	// 如果达到目的地则返回true
	if (fabs(deltaYaw) < 0.01f && requestTurnOver())
	{
		delete this;
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------
}

