/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <string>

#include "abc.h"
#include "flashevents.h"
#include "swf.h"

using namespace std;

extern __thread SystemState* sys;

Event::Event(const string& t):type(t)
{
	setVariableByQName("ENTER_FRAME","",new ASString("enterFrame"));
	setVariableByQName("ADDED_TO_STAGE","",new ASString("addedToStage"));
	setVariableByQName("INIT","",new ASString("init"));
	setVariableByQName("ADDED","",new ASString("added"));
	setVariableByQName("COMPLETE","",new ASString("complete"));
	setVariableByQName("REMOVED","",new ASString("removed"));
	setVariableByQName("UNLOAD","",new ASString("unload"));
	setVariableByQName("ACTIVATE","",new ASString("activate"));
	setVariableByQName("DEACTIVATE","",new ASString("deactivate"));
	setGetterByQName("type","",new Function(_getType));
}

FocusEvent::FocusEvent():Event("focusEvent")
{
	setVariableByQName("FOCUS_IN","",new ASString("focusIn"));
	setVariableByQName("FOCUS_OUT","",new ASString("focusOut"));
	setVariableByQName("MOUSE_FOCUS_CHANGE","",new ASString("mouseFocusChange"));
	setVariableByQName("KEY_FOCUS_CHANGE","", new ASString("keyFocusChange"));
}

KeyboardEvent::KeyboardEvent():Event("keyboardEvent")
{
	setVariableByQName("KEY_DOWN","",new ASString("keyDown"));
	setVariableByQName("KEY_UP","",new ASString("keyUp"));
}

MouseEvent::MouseEvent():Event("mouseEvent")
{
	setVariableByQName("MOUSE_DOWN","",new ASString("mouseDown"));
	setVariableByQName("MOUSE_UP","",new ASString("mouseUp"));
	setVariableByQName("CLICK","",new ASString("click"));
}

IOErrorEvent::IOErrorEvent():Event("IOErrorEvent")
{
	setVariableByQName("IO_ERROR","",new ASString("ioError"));
}

EventDispatcher::EventDispatcher():id(0)
{
	if(constructor)
		constructor->decRef();
	constructor=new Function(_constructor);
}

ASFUNCTIONBODY(Event,_getType)
{
	Event* th=static_cast<Event*>(obj);
	return new ASString(th->type);
}

ASFUNCTIONBODY(EventDispatcher,addEventListener)
{
	if(args->at(0)->getObjectType()!=T_STRING || args->at(1)->getObjectType()!=T_FUNCTION)
	{
		LOG(ERROR,"Type mismatch");
		abort();
	}
	EventDispatcher* th=dynamic_cast<EventDispatcher*>(obj);
	if(th==NULL)
		return NULL;
	sys->cur_input_thread->addListener(string(args->at(0)->toString()),th);

	IFunction* f=args->at(1)->toFunction();
	IFunction* f2=static_cast<IFunction*>(f->clone());
	f2->bind();

	th->handlers.insert(make_pair(args->at(0)->toString(),f2->toFunction()));
	sys->events_name.push_back(string(args->at(0)->toString()));
}

ASFUNCTIONBODY(EventDispatcher,dispatchEvent)
{
	EventDispatcher* th=dynamic_cast<EventDispatcher*>(obj);
	Event* e=dynamic_cast<Event*>(args->at(0));
	if(e==NULL || th==NULL)
		return new Boolean(false);
	sys->currentVm->addEvent(th,e);
	return new Boolean(true);
}

ASFUNCTIONBODY(EventDispatcher,_constructor)
{
	obj->setVariableByQName("addEventListener","",new Function(addEventListener));
	obj->setVariableByQName("dispatchEvent","",new Function(dispatchEvent));
}

void EventDispatcher::handleEvent(Event* e)
{
	map<string, IFunction*>::iterator h=handlers.find(e->type);
	if(h==handlers.end())
	{
		LOG(NOT_IMPLEMENTED,"Not handled event");
		return;
	}

	LOG(CALLS, "Handling event " << h->first);
	arguments args(1);
	args.set(0,e);
	this->incRef();
	h->second->call(this,&args);
}

