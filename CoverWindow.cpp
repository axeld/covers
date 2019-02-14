/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */


#include "CoverWindow.h"

#include <stdio.h>

#include <Bitmap.h>
#include <GridView.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <String.h>
#include <TextControl.h>
#include <TranslationUtils.h>


static const uint32 kIconSize = 64;

static const uint32 kMsgAddImage = 'adIm';
static const uint32 kMsgQuery = 'quer';


class IconView : public BView {
public:
								IconView(const char* id, BBitmap* bitmap);
	virtual						~IconView();

			void				SetSelected(bool selected);
			bool				IsSelected();

	virtual void				MouseDown(BPoint where);
	virtual void				Draw(BRect updateRect);

private:
			BString				fIdentifier;
			BBitmap*			fBitmap;
			bool				fSelected;
			float				fDescent;
};


// #pragma mark - IconView


IconView::IconView(const char* identifier, BBitmap* bitmap)
	:
	BView("image", B_WILL_DRAW),
	fIdentifier(identifier),
	fBitmap(bitmap),
	fSelected(false)
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	float width = StringWidth("WWWWWWWWWWWW");
	float height = kIconSize + be_plain_font->Size() / 12.0f * 3.0f
		+ ceilf(fontHeight.ascent + fontHeight.descent);

	fDescent = fontHeight.descent;

	SetExplicitSize(BSize(width, height));
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
	BRect bounds = Bounds();

	BRect imageBounds = bounds.InsetByCopy(4, 4);
	imageBounds.bottom = imageBounds.top + kIconSize;
	imageBounds.left = bounds.left + (bounds.Width() - kIconSize) / 2.0f;
	imageBounds.right = imageBounds.left + kIconSize;

	DrawBitmap(fBitmap, imageBounds);

	BString text = fIdentifier;
	text << " " << fBitmap->Bounds().IntegerWidth() << " x "
		<< fBitmap->Bounds().IntegerHeight();

	float width = StringWidth(text.String());
	if (width > bounds.Width()) {
		TruncateString(&text, B_TRUNCATE_END, bounds.Width());
		width = StringWidth(text.String());
	}
	BPoint location(bounds.left + (bounds.Width() - width) / 2.0f,
		bounds.bottom - fDescent);

	if (IsSelected()) {
		PushState();
		// Draw border around image
		SetHighColor(100, 180, 240);
		ClipToInverseRect(imageBounds);
//		SetHighColor(ui_color(B_LIST_SELECTED_BACKGROUND_COLOR));
		imageBounds.InsetBy(-4, -4);
		FillRect(imageBounds);
		FillRect(BRect(location.x, imageBounds.bottom,
			location.x + width, bounds.bottom));
		PopState();

		SetLowColor(140, 140, 240);
	}

	DrawString(text, location);
}


// #pragma mark - CoverWindow


CoverWindow::CoverWindow()
	:
	BWindow(BRect(0, 0, 600, 400), "Covers", B_DOCUMENT_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS),
	fQuery(NULL)
{
	fArtistControl = new BTextControl("Artist", "", new BMessage(kMsgQuery));
	fTitleControl = new BTextControl("Title", "", new BMessage(kMsgQuery));

	fMainView = new BGridView("main");
	fMainView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	BScrollView* scrollView = new BScrollView("scroller", fMainView,
		B_FULL_UPDATE_ON_RESIZE, true, true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(-2)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.Add(fArtistControl)
			.Add(fTitleControl)
		.End()
		.Add(scrollView);

	// TODO: for testing purposes only!
	const char* files[] = {"/Media/Kram/Pics/band.jpg",
		"/boot/home/Desktop/CD Cover/Antemasque - dto.jpg",
		"/boot/home/Desktop/CD Cover/Danzig - dto.jpg",
		"/boot/home/Desktop/CD Cover/Kate Bush - Never For Ever.jpg",
		"/boot/home/Desktop/animation.gif",
		"/boot/home/develop/haiku/haiku/data/artwork/HAIKU logo - white on black - normal.png",
		"/boot/home/develop/haiku/haiku/data/artwork/HAIKU square - white on blue.png", NULL};
	for (int i = 0; files[i] != NULL; i++) {
		BBitmap* bitmap = BTranslationUtils::GetBitmapFile(files[i]);
		if (bitmap == NULL)
			continue;

		BMessage add(kMsgAddImage);
		add.AddString("amazon_id", "BX348938");
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
				const char* identifier = message->GetString("amazon_id");
				fMainView->AddChild(new IconView(identifier, bitmap));
//				fMainView->GridLayout()->AddView(new IconView(bitmap), 0, 0);
			}
			break;
		}
		case kMsgQuery:
		{
			if (fQuery != NULL) {
				// Abort

				// Remove previous results
				while (BView* view = fMainView->ChildAt(0)) {
					fMainView->RemoveChild(view);
					delete view;
				}
			}

			// Start new query
			BString artist = fArtistControl->Text();
			BString title = fTitleControl->Text();
			printf("A: %s, T: %s\n", artist.String(), title.String());
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
