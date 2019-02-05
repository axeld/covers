/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "CoverWindow.h"

#include <GridView.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <TextControl.h>



CoverWindow::CoverWindow()
	:
	BWindow(BRect(0, 0, 600, 400), "Covers", B_DOCUMENT_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS)
{
	BTextControl* artistControl = new BTextControl("artist", "", NULL);
	BTextControl* titleControl = new BTextControl("title", "", NULL);

	fMainView = new BGridView("main");

	BScrollView* scrollView = new BScrollView("scroller", fMainView,
		B_FULL_UPDATE_ON_RESIZE, true, true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(artistControl)
			.Add(titleControl)
		.End()
		.Add(scrollView);
}


CoverWindow::~CoverWindow()
{
}


void
CoverWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
