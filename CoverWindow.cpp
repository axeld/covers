/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "CoverWindow.h"

#include <Bitmap.h>
#include <GridView.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <TranslationUtils.h>


static const uint32 kMsgAddImage = 'adIm';


class IconView : public BView {
public:
								IconView(BBitmap* bitmap);
	virtual						~IconView();

			void				SetSelected(bool selected);
			bool				IsSelected();

	virtual void				MouseDown(BPoint where);
	virtual void				Draw(BRect updateRect);

private:
			BBitmap*			fBitmap;
			bool				fSelected;
};


// #pragma mark - IconView


IconView::IconView(BBitmap* bitmap)
	:
	BView("image", B_WILL_DRAW),
	fBitmap(bitmap),
	fSelected(false)
{
	SetExplicitMaxSize(BSize(69, 69));
}


IconView::~IconView()
{
	delete fBitmap;
}


void
IconView::SetSelected(bool selected)
{
	if (selected == fSelected)
		return;

	fSelected = selected;
	Invalidate();
}


bool
IconView::IsSelected()
{
	return fSelected;
}


void
IconView::MouseDown(BPoint where)
{
	SetSelected(!IsSelected());
}


void
IconView::Draw(BRect updateRect)
{
	BRect rect = Bounds();
	rect.InsetBy(4, 4);
	DrawBitmap(fBitmap, rect);

	if (IsSelected()) {
		SetPenSize(7);
		SetHighColor(140, 140, 240);
//		SetHighColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR));
		StrokeRect(Bounds());
	}
}


// #pragma mark - CoverWindow


CoverWindow::CoverWindow()
	:
	BWindow(BRect(0, 0, 600, 400), "Covers", B_DOCUMENT_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS)
{
	BTextControl* artistControl = new BTextControl("Artist", "", NULL);
	BTextControl* titleControl = new BTextControl("Title", "", NULL);

	fMainView = new BGridView("main");
	fMainView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	BScrollView* scrollView = new BScrollView("scroller", fMainView,
		B_FULL_UPDATE_ON_RESIZE, true, true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(-2)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.Add(artistControl)
			.Add(titleControl)
		.End()
		.Add(scrollView);

	// TODO: for testing purposes only!
	const char* files[] = {"/Media/Kram/Pics/band.jpg",
		"/boot/home/Desktop/CD Cover/Antemasque - dto.jpg",
		"/boot/home/Desktop/CD Cover/Danzig - dto.jpg",
		"/boot/home/Desktop/CD Cover/Kate Bush - Never For Ever.jpg", NULL};
	for (int i = 0; files[i] != NULL; i++) {
		BBitmap* bitmap = BTranslationUtils::GetBitmapFile(files[i]);
		BMessage add(kMsgAddImage);
		add.AddPointer("image", bitmap);
		PostMessage(&add);
	}
}


CoverWindow::~CoverWindow()
{
}


void
CoverWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgAddImage:
		{
			BBitmap* bitmap = (BBitmap*)message->GetPointer("image");
			if (bitmap != NULL) {
				fMainView->AddChild(new IconView(bitmap));
//				fMainView->GridLayout()->AddView(new IconView(bitmap), 0, 0);
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
