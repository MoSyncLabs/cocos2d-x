/****************************************************************************
 Copyright (c) 2011      cocos2d-x.org   http://cocos2d-x.org
 Copyright (c) 2011      Максим Аксенов
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "CCApplication.h"
#include "CCEGLView.h"
#include "CCDirector.h"
#include "CCAccelerometer.h"
#include "CCTouchDispatcher.h"
#include "CCSet.h"
#include "CCTouch.h"
#include "CCTouchDispatcher.h"
#include "CCKeypadDispatcher.h"

#include <MAHeaders.h>

#define DONT_USE_TYPE_DEFINES
#include <MAFS/File.h>

#include <IX_WIDGET.h>

//#include <s3e.h>
//#include <IwMemBucketHelpers.h>

typedef struct glString_t {
	GLenum name;
	char *stringData;
} glString;

static glString sGLStrings[32];
static int sNumGLStrings = 0;

const char* glGetString(GLenum name) {
	int i, strLen;
	MAHandle data;

	for(i = 0; i < sNumGLStrings; i++) {
		if(sGLStrings[i].name == name) {
			return sGLStrings[i].stringData;
		}
	}

	data = glGetStringHandle(name);

	if(data == IOCTL_UNAVAILABLE) {
		return "";
	}

	strLen = maGetDataSize(data);
	sGLStrings[sNumGLStrings].name = name;
	sGLStrings[sNumGLStrings].stringData = (char*)malloc(strLen+1);
	maReadData(data, sGLStrings[sNumGLStrings].stringData, 0, strLen);
	sGLStrings[sNumGLStrings].stringData[strLen] = 0;
	maDestroyObject(data);
	sNumGLStrings++;
}



NS_CC_BEGIN;

// sharedApplication pointer
CCApplication * CCApplication::sm_pSharedApplication = 0;

CCApplication::CCApplication()
{
//	IW_CALLSTACK("CCApplication::CCApplication");
	
	m_nAnimationInterval = 0;
	CC_ASSERT(! sm_pSharedApplication);
	sm_pSharedApplication = this;

	m_pSet = new CCSet;

	for(int i = 0; i < 16; i++) {
		m_pTouch[i] = new CCTouch;
	}
}

CCApplication::~CCApplication()
{
//	IW_CALLSTACK("CCApplication::~CCApplication");
	
	CC_ASSERT(this == sm_pSharedApplication);		
	sm_pSharedApplication = NULL;
	CC_SAFE_DELETE(m_pSet);

	for(int i = 0; i < 16; i++) {
		CC_SAFE_DELETE(m_pTouch[i]);
	}
}


int CCApplication::Run()
{

	char buffer[1024];
	int size = maGetSystemProperty(
		"mosync.path.local",
		buffer,
		1024);

	setCurrentFileSystem(R_LOCAL_FILES, 0);
	int result = MAFS_extractCurrentFileSystem(buffer);
	freeCurrentFileSystem();

	/*
	IW_CALLSTACK("CCApplication::Run");

	
	if ( ! initInstance() || !applicationDidFinishLaunching() )
	{
		return 0;
	}
	
	int64 updateTime = s3eTimerGetMs();
	
	while (!s3eDeviceCheckQuitRequest()) 
	{ 
		int64 currentTime = s3eTimerGetMs();
		if (currentTime - updateTime > m_nAnimationInterval)
		{
			updateTime = currentTime;
			
			s3eDeviceYield(0);
			s3eKeyboardUpdate();
			s3ePointerUpdate();
			
			ccAccelerationUpdate();
			CCDirector::sharedDirector()->mainLoop();
		}
		else 
		{
			s3eDeviceYield(0);
		}
		
	}
	*/



	if ( ! initInstance() || !applicationDidFinishLaunching() )
	{
		return 0;
	}

	int updateTime = maGetMilliSecondCount();
	bool readyToRoll = false;

	while(1) {
		int currentTime = maGetMilliSecondCount();
		int diff = m_nAnimationInterval - (currentTime - updateTime);

		if (diff<=0)
		{
			updateTime = currentTime;

			//s3eDeviceYield(0);
			//s3eKeyboardUpdate();
			//s3ePointerUpdate();

			if(readyToRoll)
			{
				ccAccelerationUpdate();
				CCDirector::sharedDirector()->mainLoop();
			}
		}
		else
		{
			//s3eDeviceYield(0);
			//maWait(diff);
		}

		MAEvent event;
		while(maGetEvent(&event)) {
			switch(event.type) {
			case EVENT_TYPE_WIDGET:
					MAWidgetEventData * data = (MAWidgetEventData*) event.data;
					if(data->eventType == MAW_EVENT_GL_VIEW_READY)
					{
						CCEGLView::sharedOpenGLView().setFrameWidthAndHeight(EXTENT_X(maGetScrSize()), EXTENT_Y(maGetScrSize()));
						readyToRoll = true;
					}
				break;
			case EVENT_TYPE_CLOSE:
				return -1;
			case EVENT_TYPE_KEY_PRESSED:
				switch(event.key) {
					case MAK_MENU: CCKeypadDispatcher::sharedDispatcher()->dispatchKeypadMSG(kTypeMenuClicked); break;
					case MAK_BACK: CCKeypadDispatcher::sharedDispatcher()->dispatchKeypadMSG(kTypeBackClicked); break;
				}
				break;
			case EVENT_TYPE_POINTER_PRESSED:
				m_pTouch[event.touchId]->SetTouchInfo(0, (float)event.point.x, (float)event.point.y);
				m_pSet->addObject(m_pTouch[event.touchId]);
				cocos2d::CCDirector::sharedDirector()->getOpenGLView()->getDelegate()->touchesBegan(m_pSet, NULL);
				break;
			case EVENT_TYPE_POINTER_DRAGGED:
				m_pTouch[event.touchId]->SetTouchInfo(0, (float)event.point.x, (float)event.point.y);
				cocos2d::CCDirector::sharedDirector()->getOpenGLView()->getDelegate()->touchesMoved(m_pSet, NULL);
				break;
			case EVENT_TYPE_POINTER_RELEASED:
				m_pTouch[event.touchId]->SetTouchInfo(0, (float)event.point.x, (float)event.point.y);
				cocos2d::CCDirector::sharedDirector()->getOpenGLView()->getDelegate()->touchesEnded(m_pSet, NULL);
				m_pSet->removeObject(m_pTouch[event.touchId]);
				break;
			}
		}
	}

	return -1;
}

void CCApplication::setAnimationInterval(double interval)
{
//	IW_CALLSTACK("CCXApplication::setAnimationInterval");
	m_nAnimationInterval = 1000 * interval;
}

CCApplication::Orientation CCApplication::setOrientation(Orientation orientation)
{
//	IW_CALLSTACK("CCApplication::setOrientation");
	return orientation;
}

void CCApplication::statusBarFrame(CCRect * rect)
{
	if (rect)
	{
		*rect = CCRectMake(0, 0, 0, 0);
	}
}
void CCApplication::ccAccelerationUpdate()
{
	CCAcceleration AccValue;
//	AccValue.x = (double)s3eAccelerometerGetX()/200;
//	AccValue.y = (double)s3eAccelerometerGetY()/200;
//	AccValue.z = (double)s3eAccelerometerGetZ()/200;
	AccValue.timestamp = (double) 50/ 100;
	
	// call delegates' didAccelerate function
//	CCAccelerometer::sharedAccelerometer()->didAccelerate(&AccValue);
}
	
//////////////////////////////////////////////////////////////////////////
// static member function
//////////////////////////////////////////////////////////////////////////
CCApplication& CCApplication::sharedApplication()
{
	CC_ASSERT(sm_pSharedApplication);
	return *sm_pSharedApplication;
}

ccLanguageType CCApplication::getCurrentLanguage()
{
    return kLanguageEnglish;
}

NS_CC_END;
