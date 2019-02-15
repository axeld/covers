/* covers - Downloads cover images from Amazon.
 *
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "covers.h"

#include <Application.h>
#include <Bitmap.h>
#include <DataIO.h>
#include <HttpRequest.h>
#include <TranslationUtils.h>

#include <regex.h>
#include <stdio.h>

#include "CoverWindow.h"
#include "Utility.h"


class CoversApplication : public BApplication {
public:
								CoversApplication();
	virtual						~CoversApplication();

	virtual	void				MessageReceived(BMessage* message);
};


CoversApplication::CoversApplication()
	:
	BApplication("application/x-vnd.pinc-covers")
{
}


CoversApplication::~CoversApplication()
{
}


void
CoversApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSelected:
		case kMsgSkip:
			message->PrintToStream();
			break;
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


// #pragma mark -


int
main()
{
	CoversApplication application;

	BWindow* window = new CoverWindow();
	window->CenterOnScreen();
	window->Show();

	application.Run();
}
