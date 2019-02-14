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

#include "Query.h"
#include "Utility.h"


static const uint32 kIconSize = 64;
static const uint32 kMaxColumns = 4;

static const uint32 kMsgAddImage = 'adIm';
static const uint32 kMsgQuery = 'quer';
static const uint32 kMsgSelect = 'sele';


class ImageLoaderResultListener : public ResultListener {
public:
								ImageLoaderResultListener(
									const BMessenger& target);
	virtual						~ImageLoaderResultListener();

	virtual	void				AddResult(const BString& id,
									const char* info, UrlMap urls);

private:
			BMessenger			fTarget;
};


class IconView : public BView {
public:
								IconView(const char* identifier,
									const char* info, const char* url,
									const char* imageUrl, BBitmap* bitmap);
	virtual						~IconView();

			void				SetSelected(bool selected);
			bool				IsSelected();

	virtual void				MouseDown(BPoint where);
	virtual void				Draw(BRect updateRect);

private:
			BString				fIdentifier;
			BString				fInfo;
			BString				fUrl;
			BString				fImageUrl;
			BBitmap*			fBitmap;
			bool				fSelected;
			float				fDescent;
};


// #pragma mark - ImageLoaderResultListener


ImageLoaderResultListener::ImageLoaderResultListener(const BMessenger& target)
	:
	fTarget(target)
{
}


ImageLoaderResultListener::~ImageLoaderResultListener()
{
}


void
ImageLoaderResultListener::AddResult(const BString& id, const char* info,
	UrlMap urls)
{
	BBitmap* bitmap = NULL;

	UrlType thumbTypes[] = {kThumbImage, kLargeImage, kOriginalImage};
	for (int index = 0; index < 3; index++) {
		UrlMap::const_iterator found = urls.find(thumbTypes[index]);
		if (found == urls.end())
			continue;

		const BUrl& url = found->second;
		printf("LOAD: %s\n", url.UrlString().String());

		bitmap = FetchImage(url);
		if (bitmap != NULL)
			break;

		fprintf(stderr, "Could not load image: %s\n",
			url.UrlString().String());
	}
	if (bitmap == NULL)
		return;

	BMessage add(kMsgAddImage);
	add.AddPointer("image", bitmap);
	add.AddString("id", id);
	add.AddString("info", info);
	add.AddString("url", urls[kSource].UrlString().String());

	UrlType types[] = {kOriginalImage, kLargeImage};
	for (int index = 0; index < 2; index++) {
		UrlMap::const_iterator found = urls.find(types[index]);
		if (found == urls.end())
			continue;

		add.AddString("image_url", found->second.UrlString().String());
		break;
	}

	fTarget.SendMessage(&add);
}


// #pragma mark - IconView


IconView::IconView(const char* identifier, const char* info, const char* url,
	const char* imageUrl, BBitmap* bitmap)
	:
	BView("image", B_WILL_DRAW),
	fIdentifier(identifier),
	fInfo(info),
	fUrl(url),
	fImageUrl(imageUrl),
	fBitmap(bitmap),
	fSelected(false)
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	float width = StringWidth("WWWWWWWWWWWWWWWWWWW");
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
	int32 count = Window()->CurrentMessage()->GetInt32("clicks", 1);
	if (count == 1)
		SetSelected(!IsSelected());
	else if (count == 2) {
		BMessage select(kMsgSelect);
		select.AddString("id", fIdentifier);
		select.AddString("image_url", fImageUrl);
		Window()->PostMessage(&select);
	}
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

	BString text = fInfo;
	if (text.IsEmpty())
		text = fIdentifier;
	else
		text << " (" << fIdentifier << ")";

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
}


CoverWindow::~CoverWindow()
{
	_AbortQuery();
}


void
CoverWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgAddImage:
		{
			BBitmap* bitmap = (BBitmap*)message->GetPointer("image");
			if (bitmap != NULL) {
				const char* identifier = message->GetString("id");
				const char* info = message->GetString("info");
				const char* url = message->GetString("url");
				const char* imageUrl = message->GetString("image_url");

				IconView* view = new IconView(identifier, info, url, imageUrl,
					bitmap);
				fMainView->GridLayout()->AddView(view,
					fCount % kMaxColumns, fCount / kMaxColumns);
				fCount++;
			}
			break;
		}
		case kMsgSelect:
		{
			message->PrintToStream();
			break;
		}
		case kMsgQuery:
		{
			_AbortQuery();

			// Start new query
			BString artist = fArtistControl->Text();
			BString title = fTitleControl->Text();

			fQuery = new Query(artist, title);
			fQuery->AddListener(new ImageLoaderResultListener(this));
			fQuery->Run();
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
CoverWindow::_AbortQuery()
{
	if (fQuery != NULL) {
		// TODO: abort asynchronously in message loop!
		fQuery->Abort();
		delete fQuery;

		// Remove previous results
		while (BView* view = fMainView->ChildAt(0)) {
			fMainView->RemoveChild(view);
			delete view;
		}
		fCount++;
	}
}
